const std = @import("std");

pub const Plugin = struct {
    name: []const u8,
    init: ?fn() void = null,
    deinit: ?fn() void = null,
};

fn exampleInit() void {
    std.log.info("Example plugin initialized", .{});
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const plugin = Plugin{ .name = "example", .init = exampleInit, .deinit = null };
    if (plugin.init) |init_fn| init_fn();

    try runEditor(allocator);

    if (plugin.deinit) |deinit_fn| deinit_fn();
}

fn runEditor(alloc: std.mem.Allocator) !void {
    std.log.info("Starting editor...", .{});
    const stdin = std.io.getStdIn().reader();
    const line = try stdin.readUntilDelimiterOrEofAlloc(alloc, '\n', 1024);
    defer alloc.free(line);
    std.log.info("Received input: {s}", .{line});
}
