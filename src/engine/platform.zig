const std = @import("std");

pub const Window = struct {
    should_close: bool = false,
    pub fn destroy(self: *Window) void {
        _ = self;
        // Platform specific cleanup
    }
    pub fn shouldClose(self: *Window) bool {
        return self.should_close;
    }
};

pub const Platform = struct {
    allocator: std.mem.Allocator,
    state: *anyopaque,
    create_window: fn (*anyopaque, []const u8, u32, u32) anyerror!*Window,
    poll_events: fn (*anyopaque) void,
    deinit_fn: fn (*anyopaque) void,

    pub fn createWindow(self: *Platform, title: []const u8, width: u32, height: u32) !*Window {
        return self.create_window(self.state, title, width, height);
    }

    pub fn pollEvents(self: *Platform) void {
        self.poll_events(self.state);
    }

    pub fn deinit(self: *Platform) void {
        self.deinit_fn(self.state);
    }
};

pub fn create(allocator: std.mem.Allocator) !Platform {
    const builtin = @import("builtin");
    switch (builtin.target.os.tag) {
        .windows => return windows.init(allocator),
        .macos => return macos.init(allocator),
        else => return linux.init(allocator),
    }
}

const windows = @import("platform/windows.zig");
const macos = @import("platform/macos.zig");
const linux = @import("platform/linux.zig");
