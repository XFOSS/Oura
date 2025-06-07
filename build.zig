const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const dummy = b.addStaticLibrary(.{
        .name = "dummy",
        .target = target,
        .optimize = optimize,
    });
    dummy.linkLibC();
    dummy.addCSourceFiles(.{
        .files = &[_][]const u8{"dummy.c"},
        .flags = &[_][]const u8{"-std=c99"},
    });
    b.installArtifact(dummy);

    const ourolang = b.addExecutable(.{
        .name = "ourolang_repl",
        .target = target,
        .optimize = optimize,
    });
    ourolang.linkLibC();
    ourolang.linkLibCpp();
    ourolang.addCSourceFiles(.{
        .files = &[_][]const u8{
            "OuroLang_cpp/main.cpp",
        },
        .flags = &[_][]const u8{"-std=c++23"},
    });
    b.installArtifact(ourolang);
}
