@echo off
if exist lib_compiler\include		rd /s /q lib_compiler\include
if exist lib_compiler\src				rd /s /q lib_compiler\src
if exist lib_compiler\schemas	     rd /s /q lib_compiler\schemas
if exist lib_engine\include			rd /s /q lib_engine\include
if exist lib_engine\src				rd /s /q lib_engine\src
if exist lib_runtime\include			rd /s /q lib_runtime\include
if exist lib_runtime\src				rd /s /q lib_runtime\src
if exist lib_runtime_test\src		rd /s /q lib_runtime_test\src
if exist lib_data\include			rd /s /q lib_data\include
if exist lib_data\src					rd /s /q lib_data\src
if exist level_editor\src				rd /s /q level_editor\src
if exist level_editor\resources	rd /s /q level_editor\resources

if exist compile_server\schemas	rd /s /q compile_server\schemas
if exist compile_server\src		rd /s /q compile_server\src
if exist compile_server\data		rd /s /q compile_server\data
if exist sandbox\src					rd /s /q sandbox\src
if exist pgtech_dev\src			rd /s /q pgtech_dev\src
if exist pgtech_release\src		rd /s /q pgtech_release\src
if exist third_party					rd /s /q third_party

if exist tools\json_validator\src  rd /s /q tools\json_validator\src

mklink /j lib_compiler\include		..\..\lib_compiler\include
mklink /j lib_compiler\src			..\..\lib_compiler\src
mklink /j lib_compiler\schemas	..\..\lib_compiler\schemas
mklink /j lib_engine\include			..\..\lib_engine\include
mklink /j lib_engine\src				..\..\lib_engine\src
mklink /j lib_runtime\include		..\..\lib_runtime\include
mklink /j lib_runtime\src				..\..\lib_runtime\src
mklink /j lib_runtime_test\src		..\..\lib_runtime_test\src
mklink /j lib_data\include			..\..\lib_data\include
mklink /j lib_data\src					..\..\lib_data\src
mklink /j level_editor\src			..\..\level_editor\src
mklink /j level_editor\resources			..\..\level_editor\resources

mklink /j compile_server\schemas ..\..\lib_compiler\schemas
mklink /j compile_server\src		..\..\compile_server\src
mklink /j compile_server\data		..\..\compile_server\data
mklink /j sandbox\src				..\..\sandbox\src
mklink /j pgtech_dev\src			..\..\pgtech_dev\src
mklink /j pgtech_release\src		..\..\pgtech_release\src
mklink /j third_party					..\..\third_party
mklink /j tools\json_validator\src	..\..\tools\json_validator\src

pause