@rem Create the soft link for the third party library
@mklink /D ..\3rdParty\inc\log4cplus 	..\src\log4cplus-1.1.2\include\log4cplus
@mklink /D ..\3rdParty\inc\boost 		..\src\boost-1.55.0\boost
@mklink /D ..\3rdParty\inc\lua 			..\src\lua-5.2.3
@mklink /D ..\3rdParty\inc\gtest		..\src\gtest-1.7.0\include\gtest
@mklink /D ..\3rdParty\inc\popt			..\src\popt
@mklink /D ..\3rdParty\inc\tinyxml		..\src\tinyxml