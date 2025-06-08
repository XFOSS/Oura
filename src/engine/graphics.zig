const std = @import("std");

pub const BackendType = enum { vulkan, metal, directx12, opengl };

pub const Graphics = struct {
    allocator: std.mem.Allocator,
    backend: BackendType,

    pub fn beginFrame(self: *Graphics) void {
        _ = self;
    }

    pub fn endFrame(self: *Graphics) void {
        _ = self;
    }

    pub fn deinit(self: *Graphics) void {
        self.allocator.destroy(self);
    }
};

pub fn create(allocator: std.mem.Allocator, comptime config: anytype) !*Graphics {
    const backend: BackendType = if (@hasField(@TypeOf(config), "backend"))
        config.backend
    else
        switch (@import("builtin").target.os.tag) {
            .macos => .metal,
            .windows => .directx12,
            else => .vulkan,
        };
    const g = try allocator.create(Graphics);
    g.* = .{ .allocator = allocator, .backend = backend };
    return g;
}
