/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#import "GEM_ConfController.h"

#import <SDL/SDL_hints.h>

#include <archive.h>
#include <archive_entry.h>

#include <sys/stat.h> 
#include <stdio.h> 
#include <fcntl.h>

enum ConfigTableSection {
	TABLE_SEC_CONFIG = 0,
	TABLE_SEC_INSTALL = 1,
	TABLE_SEC_LOG = 2
	};

@implementation GEM_NavController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation) interfaceOrientation
{
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		// Don't ask me why, but these need to be reversed to revent the screen from being upsidedown.
		if (interfaceOrientation == UIDeviceOrientationLandscapeLeft)
			SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight", SDL_HINT_OVERRIDE);
		else if (interfaceOrientation == UIDeviceOrientationLandscapeRight)
			SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeLeft", SDL_HINT_OVERRIDE);
		return UIInterfaceOrientationIsLandscape(interfaceOrientation);
	}else{
		return UIInterfaceOrientationIsPortrait(interfaceOrientation);
	}
	return NO;
}

@end

@implementation GEM_ConfController
@synthesize configIndexPath;
@synthesize editor;
@synthesize controlTable;
@synthesize rootVC;
@synthesize editorVC;
@synthesize playButton;
@synthesize delegate;

- (id)init
{
    self = [super init];
    if (self) {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES); 
		docDir = [[paths objectAtIndex:0] copy]; 

		configFiles = nil;
		installFiles = nil;
		logFiles = nil;

		configIndexPath = nil;

		rootVC = nil;
		editorVC = nil;

		editorButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:@selector(saveConfig:)];
		editorButton.style = UIBarButtonItemStyleDone;
		editorButton.enabled = NO;

		UIBarButtonItem* debugBtn = [[UIBarButtonItem alloc] initWithTitle:@"Debug" style:UIBarButtonItemStyleBordered target:self action:@selector(toggleDebug:)];

		UIBarButtonItem* savesBtn = [[UIBarButtonItem alloc] initWithTitle:@"Saves" style:UIBarButtonItemStyleBordered target:self action:@selector(showSaves:)];

		UIBarButtonItem* space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];

		toolBarItems = [NSArray arrayWithObjects:debugBtn, savesBtn, space, editorButton, nil];
		[debugBtn release];
		[savesBtn release];
		[space release];
		[toolBarItems retain];

		[self reloadTableData];
    }
    return self;
}

- (void)toggleDebug:(id) __unused sender
{
	//NSString* logFile = [NSString stringWithFormat:@"%@/GemRB.%i.log", docDir, ([logFiles count] % 5) +1];
	NSString* logFile = [NSString stringWithFormat:@"%@/GemRB.log", docDir];
	//first redirect stdout and stderror to a log file in Documents so we can read it
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int log_file = open([logFile fileSystemRepresentation], O_WRONLY | O_CREAT, mode);
	
	if (dup2(log_file, fileno(stderr)) != fileno(stderr) ||
		dup2(log_file, fileno(stdout)) != fileno(stdout))
	{
		[@"Unable to redirect log output! Check the system log." writeToFile:logFile atomically:YES encoding:NSUTF8StringEncoding error:nil];
	}else{
		NSLog(@"Beginning GemRB %@ debug log.", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]);
	}
}

- (void)reloadTableData
{
	if (configFiles) [configFiles release];
	if (installFiles) [installFiles release];
	if (logFiles) [logFiles release];

	NSFileManager *manager = [NSFileManager defaultManager];
	NSArray* files = [manager contentsOfDirectoryAtPath:docDir error:nil];

	configFiles = [files filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self ENDSWITH '.cfg'"]];
	NSArray *installExtensions = [NSArray arrayWithObjects:@"bg1", @"bg2", @"tob", @"iwd", @"how", @"iwd2", @"pst", nil];
	installFiles = [files filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"pathExtension IN %@", installExtensions]];
	logFiles = [files filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self ENDSWITH '.log'"]];

	[configFiles retain];
	[installFiles retain];
	[logFiles retain];

	[controlTable reloadData];
}

- (void)dealloc
{	
	[super dealloc];
	[docDir release];
	[editorButton release];

	[configFiles release];
	[installFiles release];
	[logFiles release];

	[toolBarItems release];
}

- (void)navigationController:(UINavigationController *) __unused navigationController willShowViewController:(UIViewController *) __unused viewController animated:(BOOL) __unused animated
{
	rootVC.topViewController.toolbarItems = toolBarItems;
}

- (NSString*)selectedConfigPath
{
	if (!configIndexPath) return nil; 
	return [NSString stringWithFormat:@"%@/%@", docDir, [configFiles objectAtIndex:configIndexPath.row]];
}

- (BOOL)installGame:(NSIndexPath*)indexPath
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSString* archivePath = [installFiles objectAtIndex:indexPath.row];
	NSString* srcPath = [NSString stringWithFormat:@"%@/%@", docDir, archivePath];

	NSFileManager* fm = [NSFileManager defaultManager];

	NSError* error = nil;
	unsigned long long totalBytes = [[fm attributesOfItemAtPath:srcPath error:&error] fileSize];

	if (error) {
		NSLog(@"FM error for %@:\n%@", srcPath, [error localizedDescription]);
		return NO;
	}

	UITableViewCell* selectedCell = [controlTable cellForRowAtIndexPath:indexPath];

	UIProgressView* pv = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
	UIView* contentView = selectedCell.contentView;
	CGRect frame = CGRectMake(0.0, 0.0, contentView.frame.size.width, contentView.frame.size.height);
	dispatch_async(dispatch_get_main_queue(), ^{
		pv.frame = frame;
	});
	[contentView addSubview:pv];

	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES); 
	NSString* libDir = [[paths objectAtIndex:0] copy];

	NSString* dstPath = [NSString stringWithFormat:@"%@/%@/tmp/", libDir, [archivePath pathExtension]];
	NSLog(@"installing %@ to %@...", srcPath, dstPath);

	[fm createDirectoryAtPath:dstPath withIntermediateDirectories:YES attributes:nil error:nil];
	NSString* cwd = [fm currentDirectoryPath];
	[fm changeCurrentDirectoryPath:dstPath];

	struct archive *a;
	const char* archiveAbsPath = [srcPath cStringUsingEncoding:NSASCIIStringEncoding];
	a = archive_read_new();
	archive_read_support_format_all(a);

	struct archive *ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, 0);
	archive_write_disk_set_standard_lookup(ext);
	struct archive_entry *entry;

	//warning super bad code can cause memory leak.
	// !!! cleanup and do right
	int r;
	if ((r = archive_read_open_file(a, archiveAbsPath, 10240))) {
		NSLog(@"error opening archive (%i):%s", r, archive_error_string(a));
		return NO;
	}

	NSString* installName = nil;
	BOOL archiveHasRootDir = YES;
	unsigned long long progressSize = 0;
	for (;;) {
		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) break;
		NSString* tmp = [NSString stringWithFormat:@"%s", archive_entry_pathname(entry)];
		// Mac OS X has an annoying thing where it embeds crap in the archive. skip it.
		if ([tmp rangeOfString:@"__MACOSX/"].location != NSNotFound) continue;
		if (!installName) {
			if ([tmp rangeOfString:@"/"].location != NSNotFound) {
				installName = tmp;
			} else {
				archiveHasRootDir = NO;
			}
		} else if ([tmp rangeOfString:installName].location != 0) {
			archiveHasRootDir = NO;
		}

		if (r != ARCHIVE_OK) {
			NSLog(@"error reading archive (%i):%s", r, archive_error_string(a));
			break;
		}

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK)
			NSLog(@"%s", archive_error_string(a));
		else {
			int status;
			const void *buff;
			size_t size;
			off_t offset;

			for (;;) {
				status = archive_read_data_block(a, &buff, &size, &offset);
				if (status == ARCHIVE_EOF) break;
				if (status != ARCHIVE_OK) //need to set a return value
					break;
				progressSize += size;
				status = archive_write_data_block(ext, buff, size, offset);
				if (status != ARCHIVE_OK) {
					NSLog(@"%s", archive_error_string(a));
					break;
				}
			}

			dispatch_async(dispatch_get_main_queue(), ^{
				pv.progress = (float)((double)progressSize / (double)totalBytes);
			});
		}
	}
	archive_read_close(a);
	archive_read_finish(a);
	[fm changeCurrentDirectoryPath:cwd];

	if (r == ARCHIVE_FATAL) return NO;

	installName = [installName lastPathComponent];
	NSString* gamePath = [NSString stringWithFormat:@"%@/%@/%@/", libDir, [archivePath pathExtension], installName];
	// delete anything at gamePath. our install overwrites existing data.
	[fm removeItemAtPath:gamePath error:nil];
	if (archiveHasRootDir) {
		[fm moveItemAtPath:[NSString stringWithFormat:@"%@/%@/", dstPath, installName]
					toPath:gamePath error:nil];
		[fm removeItemAtPath:dstPath error:nil];
	} else {
		// simply rename the tmp dir to archivePath
		installName = [archivePath stringByDeletingPathExtension];
		[fm moveItemAtPath:dstPath toPath:gamePath error:nil];
	}

	NSString* savePath = [NSString stringWithFormat:@"%@/saves/%@/", libDir, [archivePath pathExtension]];
	NSString* oldSavePath = [NSString stringWithFormat:@"%@save/", gamePath];
	[fm createDirectoryAtPath:[NSString stringWithFormat:@"%@save/", savePath] withIntermediateDirectories:YES attributes:nil error:nil];
	[fm createDirectoryAtPath:[NSString stringWithFormat:@"%@mpsave/", savePath] withIntermediateDirectories:YES attributes:nil error:nil];
	[fm createDirectoryAtPath:[NSString stringWithFormat:@"%@/Caches/%@/", libDir, [archivePath pathExtension]] withIntermediateDirectories:YES attributes:nil error:nil];

	NSLog(@"moving %@ to %@", oldSavePath, savePath);
	NSArray* saves = [fm contentsOfDirectoryAtPath:oldSavePath error:nil];
	for (NSString* saveName in saves) {
		NSString* fullSavePath = [NSString stringWithFormat:@"%@save/%@", savePath, saveName];
		[fm removeItemAtPath:fullSavePath error:nil];
		[fm moveItemAtPath:[NSString stringWithFormat:@"%@/%@", oldSavePath, saveName] toPath:fullSavePath error:nil];
		NSLog(@"Moving save '%@' to %@", saveName, fullSavePath);
	}

	oldSavePath = [NSString stringWithFormat:@"%@/mpsave/", gamePath];
	saves = [fm contentsOfDirectoryAtPath:oldSavePath error:nil];

	for (NSString* saveName in saves) {
		NSString* fullSavePath = [NSString stringWithFormat:@"%@mpsave/%@", savePath, saveName];
		[fm removeItemAtPath:fullSavePath error:nil];
		[fm moveItemAtPath:[NSString stringWithFormat:@"%@/%@", oldSavePath, saveName] toPath:fullSavePath error:nil];
		NSLog(@"Moving mpsave '%@' to %@", saveName, fullSavePath);
	}

	NSString* newCfgPath = [NSString stringWithFormat:@"%@/%@.%@.cfg", docDir, installName, [archivePath pathExtension]];

	NSLog(@"Automatically creating config for %@ installed at %@ running on %i", [archivePath pathExtension], gamePath, [[UIDevice currentDevice] userInterfaceIdiom]);
	NSMutableString* newConfig = [NSMutableString stringWithContentsOfFile:@"GemRB.cfg.newinstall" encoding:NSUTF8StringEncoding error:nil];
	if ([fm fileExistsAtPath:newCfgPath]) {
		if (configIndexPath) {
			// close the config file before overwriting it.
			dispatch_async(dispatch_get_main_queue(), ^{
				[controlTable deselectRowAtIndexPath:configIndexPath animated:YES];
				[self tableView:controlTable didDeselectRowAtIndexPath:configIndexPath];
			});
		}
		// new data overwrites old data therefore new config should do the same.
		[fm removeItemAtPath:newCfgPath error:nil];
	}
	if (newConfig) {
		[newConfig appendFormat:@"\nGameType = %@", [archivePath pathExtension]];
		[newConfig appendFormat:@"\nGamePath = %@", [gamePath stringByReplacingOccurrencesOfString:libDir withString:@"../Library"]];
		// No need for CD paths
		[newConfig appendFormat:@"\nCachePath = ../Library/Caches/%@/", [archivePath pathExtension]];
		[newConfig appendFormat:@"\nSavePath = %@", [savePath stringByReplacingOccurrencesOfString:libDir withString:@"../Library"]];

		[newConfig appendString:@"\nCustomFontPath = ../Documents/"];

		NSArray* minResGames = [NSArray arrayWithObjects:@"bg1", @"pst", @"iwd", nil];
		if ([[NSPredicate predicateWithFormat:@"description IN[c] %@", minResGames] evaluateWithObject:[archivePath pathExtension]]) {
			// PST & BG1 & IWD are 640x480 without mod
			[newConfig appendString:@"\nWidth = 640"];
			[newConfig appendString:@"\nHeight = 480"];
		} else {
			if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
				[newConfig appendString:@"\nWidth = 1024"];
				[newConfig appendString:@"\nHeight = 768"];
			}else{
				[newConfig appendString:@"\nWidth = 800"];
				[newConfig appendString:@"\nHeight = 600"];
			}
		}

		NSError* err = nil;
		if (![newConfig writeToFile:newCfgPath atomically:YES encoding:NSUTF8StringEncoding error:&err]){
			NSLog(@"Unable to write config file:%@\nError:%@", newCfgPath, [err localizedDescription]);
		}
	}else{
		NSLog(@"Unable to open %@/GemRB.cfg.newinstall", cwd);
	}

	[pv removeFromSuperview];
	[pv release];
#if TARGET_IPHONE_SIMULATOR == 0
	[fm removeItemAtPath:srcPath error:nil];
#endif
	[self reloadTableData];
	[pool release];
	return YES;
}

#pragma mark TableViewDatasource

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *CellIdentifier = @"Cell";

    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
    }
	switch (indexPath.section) {
		case TABLE_SEC_CONFIG:
			cell.textLabel.text = [configFiles count] ? 
								  [configFiles objectAtIndex:indexPath.row] :
								  @"No config files found.";
			break;
		case TABLE_SEC_INSTALL:
			cell.textLabel.text = [installFiles count] ?
								  [installFiles objectAtIndex:indexPath.row] :
								  @"No game files found";
			break;
		case TABLE_SEC_LOG:
			cell.textLabel.text = [logFiles objectAtIndex:indexPath.row];
			break;
	}
	return cell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *) __unused tableView
{
	if (logFiles) return 3;
	return 2;
}

- (NSInteger)tableView:(UITableView *) __unused tableView numberOfRowsInSection:(NSInteger)section
{
	switch(section)
	{
		case TABLE_SEC_CONFIG:
			return [configFiles count] ?: 1;
		case TABLE_SEC_INSTALL:
			return [installFiles count] ?: 1;
		case TABLE_SEC_LOG:
			return [logFiles count];
	}
	return 0;
}

- (NSString *)tableView:(UITableView *) __unused tableView titleForHeaderInSection:(NSInteger)section
{
	switch(section)
	{
		case TABLE_SEC_CONFIG:
			return @"Select a configuration file.";
		case TABLE_SEC_INSTALL:
			return @"Install a game.";
		case TABLE_SEC_LOG:
			if ([logFiles count])
				return @"View a debug run log.";
	}
	return nil;
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath
{
	switch (indexPath.section) {
		case TABLE_SEC_CONFIG:
			[editor resignFirstResponder];
			[tableView cellForRowAtIndexPath:configIndexPath].accessoryType = UITableViewCellAccessoryNone;
			playButton.enabled = NO;
			editorButton.enabled = NO;
			editor.editable = NO;
			editor.text = @"";
			break;
	}
}

- (void)tableView:(UITableView *) tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	UITableViewCell* selectedCell = [tableView cellForRowAtIndexPath:indexPath];
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	[self tableView:tableView didDeselectRowAtIndexPath:indexPath];
	switch (indexPath.section) {
		case TABLE_SEC_CONFIG:
			if ([configFiles count] == 0) return;

			selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
			selectedCell.highlighted = NO;

			self.configIndexPath = indexPath;
			self.editor.text = [NSString stringWithContentsOfFile:[self selectedConfigPath] encoding:NSUTF8StringEncoding error:nil];
			if (![editor becomeFirstResponder]){
				NSLog(@"cannot set first responder");
			}

			editor.editable = YES;
			editorButton.enabled = YES;
			playButton.enabled = YES;
			if (editorVC != nil){
				editorVC.navigationItem.rightBarButtonItem = editorButton;
				[editorVC setTitle: [configFiles objectAtIndex:configIndexPath.row]];
				[rootVC pushViewController:editorVC animated:YES];
			}
			
			break;
		case TABLE_SEC_INSTALL:
			if ([installFiles count] == 0) return;
			[self performSelectorInBackground:@selector(installGame:) withObject:indexPath];
			break;
		case TABLE_SEC_LOG:
			self.editor.text = [NSString stringWithContentsOfFile:[docDir stringByAppendingFormat:@"/%@", [logFiles objectAtIndex:indexPath.row]] encoding:NSUTF8StringEncoding error:nil];
			if (editorVC != nil){
				editorVC.navigationItem.rightBarButtonItem = editorButton;
				[editorVC setTitle: [configFiles objectAtIndex:configIndexPath.row]];
				[rootVC pushViewController:editorVC animated:YES];
			}
			break;
	}
}

- (void)tableView:(UITableView *) tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (editingStyle == UITableViewCellEditingStyleInsert) return;
	
	NSFileManager* fm = [NSFileManager defaultManager];
	switch (indexPath.section) {
		case TABLE_SEC_CONFIG:
			// check if the config file we are deleting is currently selected and deselect it
			if (tableView.indexPathForSelectedRow.row == indexPath.row) {
				[tableView deselectRowAtIndexPath:indexPath animated:YES];
				[self tableView:tableView didDeselectRowAtIndexPath:indexPath];
			}
			[fm removeItemAtPath:[NSString stringWithFormat:@"%@/%@", docDir, [configFiles objectAtIndex:indexPath.row]] error:nil];
			break;
		case TABLE_SEC_INSTALL:
			[fm removeItemAtPath:[NSString stringWithFormat:@"%@/%@", docDir, [installFiles objectAtIndex:indexPath.row]] error:nil];
			break;
		case TABLE_SEC_LOG:
			[fm removeItemAtPath:[NSString stringWithFormat:@"%@/%@", docDir, [logFiles objectAtIndex:indexPath.row]] error:nil];
			break;
	}
	[self reloadTableData];
}

- (BOOL)tableView:(UITableView *) __unused tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
	switch (indexPath.section) {
		case TABLE_SEC_CONFIG:
			return (BOOL)[configFiles count];
		case TABLE_SEC_INSTALL:
			return (BOOL)[installFiles count];
		case TABLE_SEC_LOG:
			return (BOOL)[logFiles count];
	}
	return NO;
}

- (IBAction)saveConfig:(id) __unused sender
{
	if (editorVC)[rootVC popViewControllerAnimated:YES];
	if (editor.editable)
		[editor.text writeToFile:[self selectedConfigPath] atomically:YES encoding:NSUTF8StringEncoding error:nil];
}

- (IBAction)launchGEM:(id) __unused sender
{
	[[self delegate] setupComplete:[self selectedConfigPath]];
}

- (void)textViewDidBeginEditing:(UITextView *)textView
{
	[UIApplication sharedApplication].keyWindow.backgroundColor = [UIColor whiteColor];
	[UIView animateWithDuration:0.5 animations:^{
        CGRect frame = textView.frame;
		frame.size.height -= 300;
        textView.frame = frame;
    }];
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
	[UIView animateWithDuration:0.25 animations:^{
        CGRect frame = textView.frame;
		frame.size.height += 300;
        textView.frame = frame;
    }];
	[UIApplication sharedApplication].keyWindow.backgroundColor = [UIColor blackColor];
}

@end
