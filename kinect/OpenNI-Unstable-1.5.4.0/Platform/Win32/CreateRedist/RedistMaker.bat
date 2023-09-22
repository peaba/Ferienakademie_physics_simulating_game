@echo off

set MAJOR_VERSION=1
set MINOR_VERSION=5
set MAINTENANCE_VERSION=4
set BUILD_VERSION=0


@echo ************************
@echo *** ONI Redist Maker ***
@echo ************************
echo Going to build redist for version: %MAJOR_VERSION%.%MINOR_VERSION%.%MAINTENANCE_VERSION%.%BUILD_VERSION%!
@echo.

@echo ******************************************
@echo *** Erasing the old Final directory... ***
@echo ******************************************
If "%2%" == "64" (       
    set bits=64
) ELSE (    
    set bits=32
)

set finalDir=Final%bits%
set outputDir=Output%bits%

rmdir /S /q %finalDir%
rmdir /S /q %outputDir%
mkdir %finalDir%
mkdir %outputDir%


@echo.
@echo *******************************************
@echo *** Running the Redist OpenNI script...%output% ***
@echo *******************************************
IF "%1"=="" GOTO RedistNoParam
Redist_OpenNi.py %1 %2 %3 %4
goto FinishRedist
:RedistNoParam
Redist_OpenNi.py y 32 y
if not "%ERRORLEVEL%" == "0" goto ErrorScript
:FinishRedist

@echo.
@echo *************************************************
@echo *** Copying files into the %finalDir% directory... ***
@echo *************************************************
@echo.

Echo Copying the release notes...
copy  ..\..\..\Platform\Win32\Build\ReleaseNotes.txt %finalDir%\

Echo Copying the unpacked files...
xcopy /E /I ..\Redist\Bin %finalDir%\Files\Bin
xcopy /E /I ..\Redist\Bin64 %finalDir%\Files\Bin64
xcopy /E /I ..\Redist\Driver %finalDir%\Files\Driver
xcopy /E /I ..\Redist\Documentation %finalDir%\Files\Documentation
xcopy /E /I ..\Redist\Include %finalDir%\Files\Include
xcopy /E /I ..\Redist\Lib %finalDir%\Files\Lib
xcopy /E /I ..\Redist\Lib64 %finalDir%\Files\Lib64

Echo Copying the debug driver...
xcopy /E /I ..\Driver\BinVSDbg %finalDir%\DebugDriver

Echo Copying PDBs...
xcopy /I ..\Bin\Release\*.pdb %finalDir%\PDBs\32
xcopy /I ..\Bin64\Release\*.pdb %finalDir%\PDBs\64
xcopy /E /I ..\Driver\PDBs\*.pdb %finalDir%\PDBs\DRV

Echo Creating the build info...
@echo This build (V%MAJOR_VERSION%.%MINOR_VERSION%.%MAINTENANCE_VERSION%.%BUILD_VERSION%) was auto-generated by RedistMaker on %COMPUTERNAME%\%USERNAME% at %DATE% %TIME% > %finalDir%\Buildinfo.txt

If NOT "%BuildRepository%" == "" (    
    Echo Copying file to %BuildRepository%
	cd ..\..\..\Externals\PSCommon\Windows\CreateRedist    
    CopyToRepository.py ..\..\..\..\Platform\Win32\CreateRedist\%finalDir% %BuildRepository% %5 %bits% %6 %MAJOR_VERSION% %MINOR_VERSION% %MAINTENANCE_VERSION% %BUILD_VERSION%
	cd ..\..\..\..\Platform\Win32\CreateRedist
)

@echo.
@echo ***********************************************
@echo *** Finished creating the Final directory! ***
@echo ***********************************************

goto END

:ErrorCopy
@echo.
@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
@echo !!! An error occured while copying a file !!!
@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

exit /b 1
goto END

:ErrorScript
@echo.
@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
@echo !!! An error occured while running a script !!!
@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

exit /b 1
goto END

:END
@echo ********************************
@echo *** Exiting ONI Redist Maker ***
@echo ********************************