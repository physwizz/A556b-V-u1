# Copyright (C) 2024 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

bzl_library(
    name = "exec_aspect",
    srcs = ["exec_aspect.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//build/bazel_common_rules/exec/impl:exec_aspect",
    ],
)

bzl_library(
    name = "embedded_exec",
    srcs = ["embedded_exec.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        ":exec_aspect",
        "//build/bazel_common_rules/exec/impl:embedded_exec",
        "@bazel_skylib//lib:shell",
    ],
)

bzl_library(
    name = "exec",
    srcs = ["exec.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        ":exec_aspect",
        "//build/bazel_common_rules/exec/impl:exec",
        "@bazel_skylib//lib:shell",
    ],
)
