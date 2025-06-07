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
            "ouro_lang.cc",
        },
        .flags = &[_][]const u8{"-std=c++23"},
    });
    b.installArtifact(ourolang);

    const ourolang_cpp = b.addExecutable(.{
        .name = "ourolang_cpp",
        .target = target,
        .optimize = optimize,
    });
    ourolang_cpp.linkLibC();
    ourolang_cpp.linkLibCpp();
    ourolang_cpp.addCSourceFiles(.{
        .files = &[_][]const u8{"OuroLang_cpp/main.cc"},
        .flags = &[_][]const u8{"-std=c++23"},
    });
    b.installArtifact(ourolang_cpp);

    const ouroboros = b.addExecutable(.{
        .name = "ouroboros",
        .target = target,
        .optimize = optimize,
    });
    ouroboros.linkLibC();
    ouroboros.addCSourceFiles(.{
        .files = &[_][]const u8{
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/main.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/lexer.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/parser.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/ast.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/token.c",
            "Ouroboros/Ouroboros_Compiler/src/ouroboros/codegen.c",
        },
        .flags = &[_][]const u8{"-std=c11"},
    });
    ouroboros.addIncludePath(.{ .path = "Ouroboros/Ouroboros_Compiler/include" });
    b.installArtifact(ouroboros);
}
