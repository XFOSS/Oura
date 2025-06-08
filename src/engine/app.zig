const std = @import("std");
const platform = @import("platform.zig");
const graphics = @import("graphics.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer std.debug.assert(gpa.deinit() == .ok);
    const allocator = gpa.allocator();

    var plat = try platform.create(allocator);
    defer plat.deinit();

    var gfx = try graphics.create(allocator, .{});
    defer gfx.deinit();

    var window = try plat.createWindow("Zig Engine", 1280, 720);
    defer window.destroy();

    while (!window.shouldClose()) {
        plat.pollEvents();
        gfx.beginFrame();
        // TODO: issue rendering commands
        gfx.endFrame();
    }
}
