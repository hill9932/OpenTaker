rem if $(TargetExt) == ".lib" (copy $(SolutionDir)..\..\tmp\$(ProjectName)\$(Configuration)\$(TargetFileName) $(SolutionDir)..\..\lib\$(Configuration)\$(TargetFileName)) 
rem else copy $(SolutionDir)..\..\tmp\$(ProjectName)\$(Configuration)\$(TargetFileName) $(SolutionDir)..\..\bin\$(Configuration)\$(TargetFileName)

echo "$(SolutionDir)"
copy %$(SolutionDir)..\..\tmp\%$(ProjectName)\%$(Configuration)\%$(TargetFileName) %$(SolutionDir)..\..\bin\%$(Configuration)\%$(TargetFileName)

If errorlevel 1 @exit 0