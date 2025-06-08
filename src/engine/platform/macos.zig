const std = @import("std");
const platform = @import("../platform.zig");

pub const PlatformImpl = struct {
    allocator: std.mem.Allocator,
};

pub fn init(allocator: std.mem.Allocator) !platform.Platform {
    var impl = try allocator.create(PlatformImpl);
    impl.* = .{ .allocator = allocator };
    return platform.Platform{
        .allocator = allocator,
        .state = impl,
        .create_window = createWindow,
        .poll_events = pollEvents,
        .deinit_fn = deinit,
    };
}

fn createWindow(ptr: *anyopaque, title: []const u8, width: u32, height: u32) !*platform.Window {
    _ = title; _ = width; _ = height;
    const self = @ptrCast(*PlatformImpl, ptr);
    return self.allocator.create(platform.Window);
}

fn pollEvents(ptr: *anyopaque) void {
    _ = ptr;
}

fn deinit(ptr: *anyopaque) void {
    const self = @ptrCast(*PlatformImpl, ptr);
    self.allocator.destroy(self);
}
