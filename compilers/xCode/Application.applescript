(*  $Id: Application.applescript 281982 2011-05-09 15:34:15Z mcelhany $
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Author:  Vlad Lebedev
 *
 * File Description:
 * Main Application Script
 *
 *)


(* ==== Globals ==== *)
global AllLibraries
global AllConsoleTools
global AllApplications
global ToolkitSource
global ProjBuilderLib

global TheNCBIPath, TheFLTKPath, TheBDBPath, ThePCREPath, TheOUTPath
global libTypeDLL, guiLibs, zeroLink, fixContinue, xcodeTarget, projFile


(* ==== 3rd party libarary properties ==== *)
global libScriptTmpDir
global libScriptPID
property libScriptWasRun : false
property libScriptRunning : false


(* ==== Properties ==== *)
property allPaths : {"pathNCBI", "pathFLTK", "pathBDB", "pathPCRE", "pathOUT"}
property libDataSource : null
property toolDataSource : null
property appDataSource : null
property curDataSource : null

(* Loading of library scripts *)
on launched theObject
	my loadScripts()
	tell ToolkitSource to Initialize()
	
	-- Load User Defaults	
	repeat with p in allPaths
		try
			set tmp to contents of default entry (p as string) of user defaults
			set contents of text field (p as string) of tab view item "tab1" of tab view "theTab" of window "Main" to tmp
		on error
			set homePath to the POSIX path of (path to home folder) as string
			
			if (p as string) is equal to "pathNCBI" then set contents of text field (p as string) of tab view item "tab1" of tab view "theTab" of window "Main" to homePath & "c++"
			if (p as string) is equal to "pathOUT" then set contents of text field (p as string) of tab view item "tab1" of tab view "theTab" of window "Main" to homePath & "out"
		end try
		
	end repeat
	
	load nib "InstallLibsPanel"
	show window "Main"
end launched


on pathToScripts()
	set appPath to (path to me from user domain) as text
	return (appPath & "Contents:Resources:Scripts:") as text
end pathToScripts

on loadScript(scriptName)
	return load script file (my pathToScripts() & scriptName & ".scpt")
end loadScript

on loadScripts()
	set tootkitLib to my loadScript("Libraries")
	set projLib to my loadScript("ProjBuilder")
	
	set ToolkitSource to ToolkitSource of tootkitLib
	set ProjBuilderLib to ProjBuilder of projLib
end loadScripts

(* Additional Application Setup *)
(* Quit Application after main window is closed *)
on should quit after last window closed theObject
	return true
end should quit after last window closed



(* When the NIB (resourses are loaded *)
on awake from nib theObject
	if name of theObject is "Main" then
		tell theObject
			-- Set the drawer up with some initial values.
			set leading offset of drawer "Drawer" to 20
			set trailing offset of drawer "Drawer" to 20
		end tell
	end if
end awake from nib


(* Actual work starts here *)
on clicked theObject
	if name of theObject is "theLibsTable" then
		set curDataSource to libDataSource
		x_SaveTableData(libDataSource, AllLibraries)
	else if name of theObject is "theToolsTable" then
		set curDataSource to toolDataSource
		x_SaveTableData(toolDataSource, AllConsoleTools)
	else if name of theObject is "theAppsTable" then
		set curDataSource to appDataSource
		x_SaveTableData(appDataSource, AllApplications)
	end if
	
	if name of theObject is "selectAll" then
		x_SelectAll(true)
	else if name of theObject is "deselectAll" then
		x_SelectAll(false)
	end if
	
	if name of theObject is "otherLibs" then -- install 3rd party libs
		-- first, be sure the C++ Toolkit path is set correctly
		set TheNCBIPath to contents of text field "pathNCBI" of tab view item "tab1" of tab view "theTab" of window "Main"
		if x_NoSuchPath(TheNCBIPath & "/include/ncbiconf.h") then
			x_ShowAlert("NCBI C++ Toolkit was not found at " & TheNCBIPath)
			return
		end if
		
		set homePath to the POSIX path of (path to home folder) as string
		set contents of text field "tmp_dir" of window "install_libs" to homePath & "tmp"
		set contents of text field "ins_dir" of window "install_libs" to homePath & "sw"
		display panel window "install_libs" attached to window "Main"
	end if
	
	if name of theObject is "close" then
		if libScriptWasRun then
			set new_insdir to contents of text field "ins_dir" of window "install_libs"
			set contents of text field "pathFLTK" of tab view item "tab1" of tab view "theTab" of window "Main" to new_insdir
			set contents of text field "pathBDB" of tab view item "tab1" of tab view "theTab" of window "Main" to new_insdir
			set contents of text field "pathPCRE" of tab view item "tab1" of tab view "theTab" of window "Main" to new_insdir
		end if
		close panel window "install_libs"
	end if
	
	if name of theObject is "guiOpt" then
		set checked to content of theObject
		repeat with library in AllLibraries
			if gui of library is true then set req of library to checked
		end repeat
		repeat with tool in AllConsoleTools
			if gui of tool is true and checked is false then set req of tool to false -- uncheck only
		end repeat
		repeat with the_app in AllApplications
			if gui of the_app is true and checked is false then set req of the_app to false -- uncheck only
		end repeat
		
		--if libDataSource is not null then set update views of libDataSource to true
		--x_SaveTableData(libDataSource, AllLibraries)
		--x_SaveTableData(toolDataSource, AllConsoleTools)
		--x_SaveTableData(appDataSource, AllApplications)
		my x_ReloadTable(libDataSource, AllLibraries)
		my x_ReloadTable(toolDataSource, AllConsoleTools)
		my x_ReloadTable(appDataSource, AllApplications)
	end if
	
	
	if name of theObject is "do_it" then
		if libScriptRunning then -- cancel
			set enabled of button "do_it" of window "install_libs" to false -- prevent double kill
			do shell script "kill " & libScriptPID
		else
			x_Install3rdPartyLibs() -- launch new script in the background
		end if
	end if
	
	tell window "Main"
		(* Handle Generate button *)
		if theObject is equal to button "generate" then
			my x_AddtoLog("Log started: " & (current date) as text)
			
			set msg to my ValidatePaths() -- Validate paths and set globals
			if msg is "" then
				tell progress indicator "progressBar" to start
				set maximum value of progress indicator "progressBar" to (my x_GetTargetCount()) + 1
				
				try
					tell ProjBuilderLib to Initialize()
					my CreateProject() -- do the job
				on error errMsg
					log errMsg
					display dialog "Generation failed with the following message: " & return & return & errMsg buttons {"OK"} default button 1
				end try
				
				tell progress indicator "progressBar" to stop
			else
				my x_ShowAlert(msg)
			end if
		end if
		
		(* Help button pressed *)
		if theObject is equal to button "helpButton" then
			open location "http://www.ncbi.nlm.nih.gov/books/NBK7160/"
		end if
		
		(* Handle Paths *)
		tell tab view item "tab1" of tab view "theTab"
			if theObject is equal to button "ChooseNCBI" then
				my ChooseFolder("Select NCBI C++ Toolkit location", "pathNCBI")
			end if
			
			if theObject is equal to button "ChooseBDB" then
				my ChooseFolder("Select Berkley DB installation", "pathBDB")
			end if
			
			
			if theObject is equal to button "ChooseFLTK" then
				my ChooseFolder("Select FLTK installation", "pathFLTK")
			end if
			
			if theObject is equal to button "ChoosePCRE" then
				my ChooseFolder("Select PCRE and Image libraries (GIF, TIFF & PNG) installation", "pathPCRE")
			end if
			
			if theObject is equal to button "ChooseOUT" then
				my ChooseFolder("Select where the project file will be created", "pathOUT")
			end if
			
		end tell
	end tell
end clicked


(* Called right before application will quit *)
on will quit theObject
	try
		-- Save User Defaults	
		repeat with p in allPaths
			set thePath to the contents of text field (p as string) of tab view item "tab1" of tab view "theTab" of window "Main"
			make new default entry at end of default entries of user defaults with properties {name:p, contents:thePath}
			set contents of default entry (p as string) of user defaults to thePath
		end repeat
	end try
end will quit



on selected tab view item theObject tab view item tabViewItem
	if name of tabViewItem is "tab2" then
		set libTable to table view "theLibsTable" of scroll view "theLibsTable" of split view "theSplitter" of tab view item "tab2" of theObject
		set toolTable to table view "theToolsTable" of scroll view "theToolsTable" of split view "theSplitter" of tab view item "tab2" of theObject
		set appTable to table view "theAppsTable" of scroll view "theAppsTable" of split view "theSplitter" of tab view item "tab2" of theObject
		
		-- Here we will add the data columns to the data source of the contacts table view
		if libDataSource is null then
			set libDataSource to make new data source at the end of the data sources with properties {name:"libs"}
			make new data column at the end of data columns of libDataSource with properties {name:"use"}
			make new data column at the end of data columns of libDataSource with properties {name:"name"}
			set data source of libTable to libDataSource
			my x_ReloadTable(libDataSource, AllLibraries)
		end if
		
		if toolDataSource is null then
			set toolDataSource to make new data source at the end of the data sources with properties {name:"tools"}
			make new data column at the end of data columns of toolDataSource with properties {name:"use"}
			make new data column at the end of data columns of toolDataSource with properties {name:"name"}
			make new data column at the end of data columns of toolDataSource with properties {name:"path"}
			set data source of toolTable to toolDataSource
			my x_ReloadTable(toolDataSource, AllConsoleTools)
		end if
		
		if appDataSource is null then
			set appDataSource to make new data source at the end of the data sources with properties {name:"apps"}
			make new data column at the end of data columns of appDataSource with properties {name:"use"}
			make new data column at the end of data columns of appDataSource with properties {name:"name"}
			make new data column at the end of data columns of appDataSource with properties {name:"path"}
			set data source of appTable to appDataSource
			my x_ReloadTable(appDataSource, AllApplications)
		end if
	end if
end selected tab view item


on idle theObject
	if libScriptRunning then
		log "Checking status..."
		try
			set msg to do shell script "tail " & libScriptTmpDir & "/log.txt"
			set thetext to "Last few lines of " & libScriptTmpDir & "/log.txt:" & return & msg
			set contents of text view "status" of scroll view "status" of window "install_libs" to thetext
			set stat to do shell script "ps -p " & libScriptPID & " | grep " & libScriptPID
		on error
			tell progress indicator "progress" of window "install_libs" to stop
			set libScriptRunning to false
			set enabled of button "close" of window "install_libs" to true
			set title of button "do_it" of window "install_libs" to "Install third-party libraries"
			set enabled of button "do_it" of window "install_libs" to true
		end try
	end if
	return 3
end idle

on cell value theObject row theRow table column tableColumn
	(*Add your script here.*)
end cell value

on number of rows theObject
	(*Add your script here.*)
end number of rows


(** calculate the total number of targets **)
on x_GetTargetCount()
	set total to 0
	repeat with library in AllLibraries
		if req of library is true then set total to total + 1
		--repeat with lib in libs of library
		--set total to total + 1
		--end repeat
	end repeat
	
	repeat with tool in AllConsoleTools
		if req of tool is true then set total to total + 1
	end repeat
	
	repeat with theApp in AllApplications
		if req of theApp is true then set total to total + 1
	end repeat
	
	return total
end x_GetTargetCount


(* Launch shell script to install third party libraries *)
on x_Install3rdPartyLibs()
	set libScriptTmpDir to contents of text field "tmp_dir" of window "install_libs"
	if x_NoSuchPath(libScriptTmpDir) then
		display dialog "Temporary directory was not found at:" & return & libScriptTmpDir buttons {"OK"} default button 1 with icon caution
		return
	end if
	
	set libScriptRunning to true
	set libScriptWasRun to true
	tell progress indicator "progress" of window "install_libs" to start
	try
		set libScriptPID to "0"
		set toolkit_dir to contents of text field "pathNCBI" of tab view item "tab1" of tab view "theTab" of window "Main"
		set libScriptTmpDir to contents of text field "tmp_dir" of window "install_libs"
		set ins_dir to contents of text field "ins_dir" of window "install_libs"
		set theScript to toolkit_dir & "/compilers/xCode/thirdpartylibs.sh"
		
		set theScript to theScript & " " & toolkit_dir -- first argument
		set theScript to theScript & " " & libScriptTmpDir -- second argument
		set theScript to theScript & " " & ins_dir -- third argument
		if content of button "download_it" of window "install_libs" then
			set theScript to theScript & " download" -- optional download flag
		end if
		set theScript to theScript & " > " & libScriptTmpDir & "/log.txt 2>&1 & echo $!" -- to log file
		
		set title of button "do_it" of window "install_libs" to "Cancel"
		set enabled of button "close" of window "install_libs" to false
		log theScript
		set libScriptPID to do shell script theScript -- start background process
		log "Launched PID: " & libScriptPID
	end try
end x_Install3rdPartyLibs


(* Select a directory with given title *)
on ChooseFolder(theTitle, textField)
	tell open panel
		set title to theTitle
		set prompt to "Choose"
		set treat packages as directories to true
		set can choose directories to true
		set can choose files to false
		set allows multiple selection to false
	end tell
	
	set theResult to display open panel in directory "~" with file name ""
	if theResult is 1 then
		set pathNames to (path names of open panel as list)
		set thePath to the first item of pathNames
		log thePath
		set contents of text field textField of tab view item "tab1" of tab view "theTab" of window "Main" to thePath
	end if
	
end ChooseFolder


on x_ReloadTable(theDS, thePack)
	if theDS is null then return
	
	set update views of theDS to false
	delete every data row in theDS
	
	repeat with p in thePack
		set theDataRow to make new data row at the end of the data rows of theDS
		set contents of data cell "use" of theDataRow to req of p
		set contents of data cell "name" of theDataRow to name of p
		if theDS is not equal to libDataSource then set contents of data cell "path" of theDataRow to path of p
	end repeat
	
	set update views of theDS to true
end x_ReloadTable


(* Append to log entry *)
on x_AddtoLog(txt)
	tell window "Main"
		set tmp to contents of text view "logView" of scroll view "scrollView" of drawer "Drawer"
		set contents of text view "logView" of scroll view "scrollView" of drawer "Drawer" to txt & return & tmp
	end tell
	log txt
end x_AddtoLog


(* Actual work happends here *)
on CreateProject()
	repeat with library in AllLibraries -- ncbi_core : {name:"ncbi_core", libs:{xncbi, xcompress, tables, sequtil, creaders, xutil, xregexp, xconnect, xser}}
		if req of library is true then -- selected to be build
			set src_files to {}
			set hdr_files to {}
			repeat with lib in libs of library
				set src_files to src_files & my GetSourceFiles(lib)
				set hdr_files to hdr_files & my GetHeaderFiles(lib)
				x_AddtoLog("Processing: " & name of lib)
			end repeat
			
			x_IncrementProgressBar()
			--if name of library = "ncbi_core" then --then --or name of lib = "xser" or name of lib = "xutil" or name of lib = "access" then
			tell ProjBuilderLib to MakeNewLibraryTarget(library, src_files, hdr_files)
			--end if
		end if -- req is true
	end repeat
	
	--repeat with toolBundle in AllConsoleTools
	repeat with tool in AllConsoleTools --toolBundle
		if req of tool is true then -- selected to be build
			set src_files to my GetSourceFiles(tool)
			set hdr_files to hdr_files & my GetHeaderFiles(lib)
			x_AddtoLog("Processing: " & name of tool)
			x_IncrementProgressBar()
			tell ProjBuilderLib to MakeNewToolTarget(tool, src_files, hdr_files)
		end if
	end repeat
	--end repeat
	
	--repeat with appBundle in AllApplications
	repeat with theApp in AllApplications --appBundle
		if req of theApp is true then -- selected to be build
			set src_files to my GetSourceFiles(theApp)
			set hdr_files to my GetHeaderFiles(lib)
			x_AddtoLog("Processing: " & name of theApp)
			x_IncrementProgressBar()
			tell ProjBuilderLib to MakeNewAppTarget(theApp, src_files, hdr_files)
		end if
	end repeat
	--end repeat
	
	x_AddtoLog("Saving project file")
	tell ProjBuilderLib to SaveProjectFile()
	
	x_IncrementProgressBar()
	x_AddtoLog("Opening generated project: " & TheOUTPath & "/" & projFile)
	do shell script "open " & TheOUTPath & "/" & projFile -- Open Project
	x_AddtoLog("Done")
	
	tell application "Xcode"
		set ver to version
	end tell
	if ver � "1.5" then
		if content of button "buildProj" of window "Main" is true then -- build the new project
			x_AddtoLog("Building")
			tell application "Xcode"
				activate
				tell application "System Events" -- it's a little hack, but it's work
					keystroke "b" using command down
				end tell
			end tell
			--tell application "Xcode" to build project "NCBI" -- not yet implemented in xCode 1.2
		end if
	else
		my x_ShowAlert("xCode version 1.5 or greater is required. Please visit  www.apple.com/developer  to download the latest version.")
	end if
	
end CreateProject


on x_IncrementProgressBar()
	tell window "Main"
		tell progress indicator "progressBar" to increment by 1
	end tell
end x_IncrementProgressBar



(* Retriece a list of source files based on library info *)
on GetSourceFiles(lib)
	set fullSourcePath to TheNCBIPath & "/src/"
	set incfileList to {}
	set excfileList to {}
	set src_files to {}
	
	try -- Try to get main path
		set fullSourcePath to fullSourcePath & x_Replace((path of lib), ":", "/") & "/"
	end try
	
	try -- Try to get the included file list
		set incfileList to inc of lib
		repeat with F in incfileList
			set src_files to src_files & (fullSourcePath & F)
		end repeat
		return src_files -- done
	end try
	
	
	try -- Try to get the excluded file list
		set excfileList to exc of lib
	end try
	
	-- Get everything in this path
	set src_files to x_GetFolderContent(fullSourcePath, excfileList)
	
	if name of lib is "xncbi" then copy TheOUTPath & "/cfg/ncbicfg.c" to the end of src_files
	return src_files
end GetSourceFiles


(* Retriece a list of header files based on library info *)
on GetHeaderFiles(lib)
	set fullHeaderPath1 to TheNCBIPath & "/include/"
	set fullHeaderPath2 to TheNCBIPath & "/src/"
	set endsList to {".h", ".hpp"}
	set hdr_files to {}
	
	try -- Try to get main path
		set fullHeaderPath1 to fullHeaderPath1 & x_Replace((path of lib), ":", "/") & "/"
		set fullHeaderPath2 to fullHeaderPath2 & x_Replace((path of lib), ":", "/") & "/"
	end try
	
	try -- get headers from the include folder
		set fileList to list folder (fullHeaderPath1 as POSIX file) without invisibles
		set fileList to EndsWith(fileList, endsList, "_.hpp")
		repeat with F in fileList
			copy fullHeaderPath1 & F to the end of hdr_files
		end repeat
	end try
	
	try -- get headers from the src folder
		set fileList to list folder (fullHeaderPath2 as POSIX file) without invisibles
		set fileList to EndsWith(fileList, endsList, "_.hpp")
		repeat with F in fileList
			copy fullHeaderPath2 & F to the end of hdr_files
		end repeat
	end try
	
	return hdr_files
end GetHeaderFiles


(* Returns a content of a foder, with *.c *.c.in and *.cpp files, excluding "excfileList"  and full path *)
on x_GetFolderContent(folderName, excfileList)
	set fileList to list folder (folderName as POSIX file) without invisibles
	set fileList to my ExcludeFiles(fileList, excfileList)
	set endsList to {".c", ".cpp", ".c.in"}
	
	set fileList to EndsWith(fileList, endsList, "_.cpp")
	
	set filesWithPath to {}
	repeat with F in fileList
		copy folderName & F to the end of filesWithPath
	end repeat
	return filesWithPath
end x_GetFolderContent

(* Returns a new list with items "allFiles" excluding "excFiles" *)
on ExcludeFiles(allFiles, excFiles)
	set newList to {}
	repeat with F in allFiles
		if excFiles does not contain F then
			copy F to the end of newList
		end if
	end repeat
	return newList
end ExcludeFiles


(*  Replace all occurances of "old" in the aString with "new" *)
on x_Replace(aString, old, new)
	set OldDelims to AppleScript's text item delimiters
	set AppleScript's text item delimiters to old
	set newText to text items of aString
	set AppleScript's text item delimiters to new
	set finalText to newText as text
	set AppleScript's text item delimiters to OldDelims
	
	return finalText
end x_Replace


(* Return a subset of "aList" with items ending with "suffix" *)
on EndsWith(aList, suffixList, exclude)
	set newList to {}
	repeat with F in aList
		repeat with S in suffixList
			if (F ends with S) and (F does not end with exclude) then
				copy (F as string) to end of newList
			end if
		end repeat
	end repeat
	return newList
end EndsWith


(* Performs a validation of paths and names before generating a project *)
on ValidatePaths()
	tell tab view item "tab1" of tab view "theTab" of window "Main"
		set TheNCBIPath to contents of text field "pathNCBI"
		set TheFLTKPath to contents of text field "pathFLTK"
		set TheBDBPath to contents of text field "pathBDB"
		set ThePCREPath to contents of text field "pathPCRE"
		set TheOUTPath to contents of text field "pathOUT"
	end tell
	
	set ncbiPath to x_Replace(TheNCBIPath, "/", ":")
	
	if TheNCBIPath is "" or TheFLTKPath is "" or TheBDBPath is "" or ThePCREPath is "" or TheOUTPath is "" then
		return "Path(s) could not be empty"
	end if
	
	-- check paths"
	if x_NoSuchPath(TheNCBIPath & "/include/ncbiconf.h") then
		return "NCBI C++ Toolkit was not found at " & TheNCBIPath
	end if
	
	if x_NoSuchPath(ThePCREPath & "/include/tiff.h") then
		return "Lib TIFF installation was not found at " & ThePCREPath
	end if
	
	--if x_NoSuchPath(ThePCREPath & "/include/jpeglib.h") then
	--	return "Lib JPEG installation was not found at " & ThePCREPath
	--end if
	
	if x_NoSuchPath(ThePCREPath & "/include/png.h") then
		return "Lib PNG installation was not found at " & ThePCREPath
	end if
	
	if x_NoSuchPath(ThePCREPath & "/include/gif_lib.h") then
		--return "Lib GIF installation was not found at " & ThePCREPath
	end if
	
	if x_NoSuchPath(TheBDBPath & "/include/db.h") then
		return "Berkeley DB installation was not found at " & TheBDBPath
	end if
	
	if x_NoSuchPath(TheFLTKPath & "/include/FL/Fl.H") then
		return "FLTK installation was not found at " & TheFLTKPath
	end if
	
	if x_NoSuchPath(TheOUTPath) then
		do shell script "mkdir " & TheOUTPath
		x_AddtoLog("The Output folder was created at: " & TheOUTPath)
		--return "The Output folder was not found at: " & TheOUTPath
	end if
	
	-- create (or re-create) the cfg directory
	if x_NoSuchPath(TheOUTPath & "/cfg") then
		do shell script "mkdir " & TheOUTPath & "/cfg"
		set lib_dir to TheOUTPath & "/lib"
		set lib_dir to x_Replace(lib_dir, "/", "\\/")
		set ncbicfg to "sed 's/@ncbi_runpath@/" & lib_dir & "/' <" & TheNCBIPath & "/src/corelib/ncbicfg.c.in >" & TheOUTPath & "/cfg/ncbicfg.c"
		
		do shell script ncbicfg
	end if
	
	set libTypeDLL to true --content of button "libType" of window "Main" -- DLL or Static
	set guiLibs to content of button "guiOpt" of window "Main" -- CPU specific optimization
	set zeroLink to false --content of button "zeroLink" of window "Main" -- Use Zero Link
	set fixContinue to content of button "fixCont" of window "Main" -- Use Fix & Continue
	set xcodeTarget to current row of matrix "xcodeTar" of window "Main" -- Debug / Release
	
	
	set projFile to "NCBI.xcodeproj"
	
	return "" -- no errors found
end ValidatePaths


(* Checks if path/file exists *)
on x_NoSuchPath(thePath)
	set posix_path to thePath as POSIX file
	try
		info for posix_path
		return false
	on error
		return true
	end try
end x_NoSuchPath


(* Display a message box *)
on x_ShowAlert(msg)
	log msg
	display dialog msg buttons {"OK"} default button 1 attached to window "Main" with icon caution
end x_ShowAlert


on x_SelectAll(theBool)
	if curDataSource is not null then
		repeat with d in data rows of curDataSource
			set contents of data cell "use" of d to theBool
		end repeat
		
		x_SaveTableData(libDataSource, AllLibraries)
		x_SaveTableData(toolDataSource, AllConsoleTools)
		x_SaveTableData(appDataSource, AllApplications)
	end if
end x_SelectAll


on x_SaveTableData(theDS, thePack)
	if theDS is null then return
	set c to 1
	repeat with p in thePack
		set theDataRow to item c of the data rows of theDS
		set req of p to contents of data cell "use" of theDataRow
		
		set c to c + 1
	end repeat
end x_SaveTableData
