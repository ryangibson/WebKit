#!/bin/sh

CONTENTS_MODULES_DIR=${BUILT_PRODUCTS_DIR}/${JAVASCRIPTCORE_CONTENTS_DIR}/Modules

if [ ! -d "${CONTENTS_MODULES_DIR}" ]; then
    mkdir -p "${CONTENTS_MODULES_DIR}"
fi
cp  ${MODULEMAP_FILE} ${CONTENTS_MODULES_DIR}/module.modulemap

if [ ! -e "${BUILT_PRODUCTS_DIR}/${PRODUCT_MODULE_NAME}.framework/Modules" ]; then
    pushd "${BUILT_PRODUCTS_DIR}/${PRODUCT_MODULE_NAME}.framework"
    ln -s "Versions/Current/Modules"
    popd
fi

OUTPUT_DIR="/Users/gibsonry/Library/Mobile Documents/com~apple~CloudDocs/rpg/repos/projects/apps/ScriptThing/Frameworks"
rm -fr "${OUTPUT_DIR}/JavaScriptCore.framework"
cp -R "${BUILT_PRODUCTS_DIR}/${PRODUCT_MODULE_NAME}.framework" "${OUTPUT_DIR}/"

install_name_tool -id  @executable_path/../Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore "${OUTPUT_DIR}/JavaScriptCore.framework/JavaScriptCore"
