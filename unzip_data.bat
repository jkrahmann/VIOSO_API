@echo off
setlocal
echo %cd%
if exist "..\data\vioso3D.vwf" (
    rem "file already unzipped"
) else (
    tar xzf "..\data\vioso3D.zip" -C "..\..\data"
)
if exist "..\data\vioso2d.vwf" (
    rem "file already unzipped"
) else (
    tar xzf "..\data\vioso2d.zip" -C "..\..\data"
)
exit /b
