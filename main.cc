#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <span>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#define namespace ns
#include <wlr-layer-shell-unstable-v1-client-protocol.h>
#undef namespace

#define app_name "loup"

auto check_null_impl(auto ptr, char const *info) {
  if (ptr == nullptr) [[unlikely]] {
    std::fputs(info, stderr);
    exit(1);
  }
  return ptr;
}

#define check_null(...) check_null_impl(__VA_ARGS__, #__VA_ARGS__ " is null\n")

auto address_of(auto const& x) noexcept {
  return std::addressof(x);
}

void nop(auto...) noexcept {}

struct window {
  wl_surface *surface;
  zwlr_layer_surface_v1 *layer_surface;

  std::uint32_t width;
  std::uint32_t height;

  void on_layer_surface_configure(std::uint32_t width, std::uint32_t height) noexcept {
    this->width = width;
    this->height = height;
  }

  auto buffer_size() const noexcept {
    return this->stride() * this->height;
  }

  auto stride() const noexcept -> std::uint32_t {
    return this->width * 4;
  }

  auto commit_surface() const noexcept {
    wl_surface_commit(this->surface);
  }
};

#define reflect \
  dispatch(wl_compositor, compositor) \
  dispatch(zwlr_layer_shell_v1, layer_shell) \
  dispatch(wl_shm, shm)
//

struct globals {
#define dispatch(type, iden) type *iden;
  reflect
#undef dispatch

  void bind(
    wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version
  ) noexcept {
#define dispatch(type, iden) \
  if (std::strcmp(interface, type##_interface.name) == 0) { \
    this->iden = static_cast<type*>(check_null(wl_registry_bind(registry, name, &type##_interface, version))); \
  } \
//
    reflect
#undef dispatch
  }

  auto create_window() const noexcept {
    const auto surface = check_null(wl_compositor_create_surface(this->compositor));
    const auto layer_surface = check_null(zwlr_layer_shell_v1_get_layer_surface(
      this->layer_shell,
      surface,
      nullptr,
      ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
      app_name
    ));

    zwlr_layer_surface_v1_set_anchor(
      layer_surface,
      0b1111
    );

    zwlr_layer_surface_v1_set_exclusive_zone(
      layer_surface,
      -1
    );
    struct window w;
    w.surface = surface;
    w.layer_surface = layer_surface;
    return w;
  }
};

#undef reflect

struct client {
  struct globals globals;
  wl_display *display;
  struct window window;
};

int main() {
  struct client client;

  client.display = check_null(wl_display_connect(nullptr));

  auto registry = check_null(wl_display_get_registry(client.display));
  wl_registry_add_listener(registry, address_of(wl_registry_listener {
    .global = [](
      void* data,
      wl_registry* registry,
      uint32_t name,
      const char* interface,
      uint32_t version
    ){
      auto& globals = *static_cast<struct globals*>(data);
      globals.bind(registry, name, interface, version);
    },
    .global_remove = nop,
  }), &client.globals);

  wl_display_roundtrip(client.display);
  wl_registry_destroy(registry);

  client.window = client.globals.create_window();
  client.window.commit_surface();

  zwlr_layer_surface_v1_add_listener(
    client.window.layer_surface,
    address_of(zwlr_layer_surface_v1_listener {
      .configure = [](
        void* data,
        zwlr_layer_surface_v1* layer_surface,
        uint32_t serial,
        uint32_t width,
        uint32_t height
      ) {
        auto& window = *static_cast<struct window*>(data);
        window.on_layer_surface_configure(width, height);
        zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
      },
      .closed = nop,
    }),
    &client.window
  );
  wl_display_roundtrip(client.display);

  char path_buf[32];
  constexpr decltype(auto) key = "XDG_RUNTIME_DIR";
  for (auto it = environ; *it; ++it) {
    const auto entry = std::string_view{*it};
    if (entry.starts_with(key)) {
      const auto value = entry.substr(std::size(key));
      std::sprintf(path_buf, "%s/" app_name, value.data());
      goto path_buf_done;
    }
  }
  std::sprintf(path_buf, "/run/user/%d/" app_name, getuid());
path_buf_done:

  const auto fd = open(path_buf, O_RDWR, S_IXOTH);
  if (fd == -1) {
    std::perror("open");
    return 1;
  }
  const auto buf_size = client.window.buffer_size();
  ftruncate(fd, buf_size);
  auto memory = static_cast<std::byte*>(check_null(mmap(
    nullptr,
    buf_size,
    PROT_READ,
    MAP_SHARED | MAP_PRIVATE,
    fd,
    0
  )));
  auto view = std::span{memory, buf_size};
  std::ignore = view;

  const auto pool = check_null(wl_shm_create_pool(
    client.globals.shm,
    fd,
   buf_size
  ));
  const auto buffer = check_null(wl_shm_pool_create_buffer(
    pool,
    0,
    client.window.width,
    client.window.height,
    client.window.stride(),
    WL_SHM_FORMAT_ARGB8888
  ));
  wl_surface_attach(client.window.surface, buffer, 0, 0);
  client.window.commit_surface();

  while (wl_display_dispatch(client.display) != -1) {
    nop();
  }
}
