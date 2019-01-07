#!/bin/sh

JS_CORE_PATH=WebKitBuild/Release/JavaScriptCore.framework
SCRIPT_THING_PATH=/Users/gibsonry/Library/Mobile\ Documents/com~apple~CloudDocs/rpg/repos/projects/apps/ScriptThing/Frameworks/

echo "cp -r ${JS_CORE_PATH} ${SCRIPT_THING_PATH}"
cp -r "${JS_CORE_PATH}" "${SCRIPT_THING_PATH}"

BINARY_PATH=${SCRIPT_THING_PATH}/JavaScriptCore.framework/JavaScriptCore
install_name_tool -id @executable_path/../Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore "${BINARY_PATH}"
otool -L "${BINARY_PATH}"

