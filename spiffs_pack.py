# import os
# import shutil
# from SCons.Script import DefaultEnvironment

# env = DefaultEnvironment()
# platform = env.PioPlatform()
# framework_dir = platform.get_package_dir("framework-arduinoespressif32")

# def copy_web_pages_to_spiffs(source, target, env):
#     spiffs_dir = os.path.join(framework_dir, "tools", "sdk", "spiffsgen", "spiffs_image")
#     web_pages_dir = os.path.join(env["PROJECT_DIR"], "data")
#     if not os.path.exists(spiffs_dir):
#         os.makedirs(spiffs_dir)
#     for root, dirs, files in os.walk(web_pages_dir):
#         for file in files:
#             print("pack file=" + file)
#             src_file = os.path.join(root, file)
#             rel_path = os.path.relpath(src_file, web_pages_dir)
#             dst_file = os.path.join(spiffs_dir, rel_path)
#             dst_dir = os.path.dirname(dst_file)
#             print("src_file=" + src_file);
#             if not os.path.exists(dst_dir):
#                 os.makedirs(dst_dir)
#             shutil.copy2(src_file, dst_file)

# print("add post action $BUILD_DIR/${PROGNAME}.elf")
# env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", copy_web_pages_to_spiffs)