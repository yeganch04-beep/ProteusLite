include("E:/ProteusLite/build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ProteusLite-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "E:/ProteusLite/build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug/ProteusLite.exe"
    GENERATE_QT_CONF
)
