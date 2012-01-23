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
#include <archive.h>
#include <archive_entry.h>

#include <sys/stat.h> 
#include <stdio.h> 
#include <fcntl.h>

@implementation GEM_NavController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation) interfaceOrientation
{
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
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

- (id)init
{
    self = [super init];
    if (self) {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES); 
		docDir = [[paths objectAtIndex:0] copy]; 

		finished = NO;

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
		NSLog(@"Beginning GemRB debug log.");
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
	[self launchGEM:nil];
	[super dealloc];
	[docDir release];
	[editorButton release];

	[configFiles release];
	[installFiles release];
	[logFiles release];

	[toolBarItems release];
}

- (void)runModal
{
	rootVC.topViewController.toolbarItems = toolBarItems;
	while (!finished) {
		// for some reason the runloop magically returns
		// after scrolling a scroll view
		// so we need to restart it
		CFRunLoopRun();
	}
}

- (void)updateProgressView:(UIProgressView*)pv
{
	[pv setNeedsDisplay];
}

- (NSString*)selectedConfigPath
{
	if (!configIndexPath) return nil; 
	return [NSString stringWithFormat:@"%@/%@", docDir, [configFiles objectAtIndex:configIndexPath.row]];
}

- (BOOL)installGame:(NSIndexPath*)indexPath
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSString* zipPath = [installFiles objectAtIndex:indexPath.row];
	NSString* srcPath = [NSString stringWithFormat:@"%@/%@", docDir, zipPath];

	NSFileManager* fm = [NSFileManager defaultManager];

	NSError* error = nil;
	unsigned long long totalBytes = [[fm attributesOfItemAtPath:srcPath error:&error] fileSize];

	if (error) NSLog(@"FM error for %@:\n%@", srcPath, [error localizedDescription]);

	UITableViewCell* selectedCell = [controlTable cellForRowAtIndexPath:indexPath];

	UIProgressView* pv = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
	UIView* contentView = selectedCell.contentView;
	CGRect frame = CGRectMake(0.0, 0.0, contentView.frame.size.width, contentView.frame.size.height);
	pv.frame = frame;
	[contentView addSubview:pv];

	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES); 
	NSString* libDir = [[paths objectAtIndex:0] copy];

	NSString* dstPath = [NSString stringWithFormat:@"%@/%@/", libDir, [zipPath pathExtension]];
	NSLog(@"installing %@ to %@...", srcPath, dstPath);

	[fm createDirectoryAtPath:dstPath withIntermediateDirectories:YES attributes:nil error:nil];
	NSString* cwd = [fm currentDirectoryPath];
	[fm changeCurrentDirectoryPath:dstPath];

	struct archive *a;
	const char* archivePath = [srcPath cStringUsingEncoding:NSASCIIStringEncoding];
	a = archive_read_new();
	archive_read_support_format_zip(a);
	
	struct archive *ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, 0);
	archive_write_disk_set_standard_lookup(ext);
	struct archive_entry *entry;

	int do_extract = 1;
	//warning super bad code can cause memory leak.
	// !!! cleanup and do right
	int r;
	if ((r = archive_read_open_file(a, archivePath, 10240))) {
		NSLog(@"error opening ZIP (%i):%s", r, archive_error_string(a));
		return NO;
	}
	NSString* installPath = nil;
	NSString* installName = nil;
	unsigned long long progressSize = 0;
	for (;;) {
		r = archive_read_next_header(a, &entry);
		if (!installName) installName = [NSString stringWithFormat:@"%s", archive_entry_pathname(entry)];
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			NSLog(@"error reading ZIP (%i):%s", r, archive_error_string(a));
			break;
		}

		if (do_extract) {
			//NSLog(@"%s", archive_entry_pathname(entry));
			r = archive_write_header(ext, entry);
			if (r != ARCHIVE_OK)
				NSLog(@"%s", archive_error_string(a));
			else{				
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
				pv.progress = (float)((double)progressSize / (double)totalBytes);
				
				[self performSelectorOnMainThread:@selector(updateProgressView:) withObject:pv waitUntilDone:NO];
			}
		}
	}
	archive_read_close(a);
	archive_read_finish(a);

	installName = [installName stringByReplacingOccurrencesOfString:@"/" withString:@""];
	installPath = [dstPath stringByAppendingString:installName];

	[fm changeCurrentDirectoryPath:cwd];

	NSString* savePath = [NSString stringWithFormat:@"%@/saves/%@", libDir, [zipPath pathExtension]];
	NSString* oldSavePath = [NSString stringWithFormat:@"%@/save/", installPath];
	[fm createDirectoryAtPath:[NSString stringWithFormat:@"%@/save/", savePath] withIntermediateDirectories:YES attributes:nil error:nil];
	[fm createDirectoryAtPath:[NSString stringWithFormat:@"%@/mpsave/", savePath] withIntermediateDirectories:YES attributes:nil error:nil];
	[fm createDirectoryAtPath:[NSString stringWithFormat:@"%@/Caches/%@/", libDir, [zipPath pathExtension]] withIntermediateDirectories:YES attributes:nil error:nil];

	NSArray* saves = [fm contentsOfDirectoryAtPath:oldSavePath error:nil];
	for (NSString* saveName in saves) {
		NSString* fullSavePath = [NSString stringWithFormat:@"%@/save/%@", savePath, saveName];
		[fm removeItemAtPath:fullSavePath error:nil];
		[fm moveItemAtPath:[NSString stringWithFormat:@"%@/%@", oldSavePath, saveName] toPath:fullSavePath error:nil];
		NSLog(@"Moving save '%@' to %@", saveName, fullSavePath);
	}

	oldSavePath = [NSString stringWithFormat:@"%@/mpsave/", installPath];
	saves = [fm contentsOfDirectoryAtPath:oldSavePath error:nil];

	for (NSString* saveName in saves) {
		NSString* fullSavePath = [NSString stringWithFormat:@"%@/mpsave/%@", savePath, saveName];
		[fm removeItemAtPath:fullSavePath error:nil];
		[fm moveItemAtPath:[NSString stringWithFormat:@"%@/%@", oldSavePath, saveName] toPath:fullSavePath error:nil];
		NSLog(@"Moving mpsave '%@' to %@", saveName, fullSavePath);
	}

	NSString* newCfgPath = [NSString stringWithFormat:@"%@/%@.%@.cfg", docDir, installName, [zipPath pathExtension]];
	if (![fm fileExistsAtPath:newCfgPath]){
		NSLog(@"Automatically creating config for %@ installed at %@ running on %i", [zipPath pathExtension], installPath, [[UIDevice currentDevice] userInterfaceIdiom]);
		NSMutableString* newConfig = [NSMutableString stringWithContentsOfFile:@"GemRB.cfg.newinstall" encoding:NSUTF8StringEncoding error:nil];

		if (newConfig) {
			[newConfig appendString:[NSString stringWithFormat:@"\nGameType = %@", [zipPath pathExtension]]];
			[newConfig appendString:[NSString stringWithFormat:@"\nGamePath = %@/", installPath]];
			[newConfig appendString:[NSString stringWithFormat:@"\nCD1 = %@/CD1/", installPath]];
			[newConfig appendString:[NSString stringWithFormat:@"\nCD2 = %@/CD2/", installPath]];
			[newConfig appendString:[NSString stringWithFormat:@"\nCD3 = %@/CD3/", installPath]];
			[newConfig appendString:[NSString stringWithFormat:@"\nCD4 = %@/CD4/", installPath]];
			[newConfig appendString:[NSString stringWithFormat:@"\nCD5 = %@/CD5/", installPath]];
			[newConfig appendString:[NSString stringWithFormat:@"\nCachePath = %@/Caches/%@/", libDir, [zipPath pathExtension]]];
			[newConfig appendString:[NSString stringWithFormat:@"\nSavePath = %@/", savePath]];

			if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
				[newConfig appendString:@"\nWidth = 1024"];
				[newConfig appendString:@"\nHeight = 768"];
			}else{
				[newConfig appendString:@"\nWidth = 800"];
				[newConfig appendString:@"\nHeight = 600"];
			}

			NSError* err = nil;
			if (![newConfig writeToFile:newCfgPath atomically:YES encoding:NSUTF8StringEncoding error:&err]){
				NSLog(@"Unable to write config file:%@\nError:%@", newCfgPath, [err localizedDescription]);
			}
		}else{
			NSLog(@"Unable to open %@/GemRB.cfg.newinstall", cwd);
		}
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
		case 0:
			cell.textLabel.text = [configFiles count] ? 
								  [configFiles objectAtIndex:indexPath.row] :
								  @"No config files found.";
			break;
		case 1:
			cell.textLabel.text = [installFiles count] ?
								  [installFiles objectAtIndex:indexPath.row] :
								  @"No game files found";
			break;
		case 2:
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
		case 0:
			return [configFiles count] ?: 1;
		case 1:
			return [installFiles count] ?: 1;
		case 2:
			return [logFiles count];
	}
	return 0;
}

- (NSString *)tableView:(UITableView *) __unused tableView titleForHeaderInSection:(NSInteger)section
{
	switch(section)
	{
		case 0:
			return @"Select a configuration file.";
		case 1:
			return @"Install a game.";
		case 2:
			if ([logFiles count])
				return @"View a debug run log.";
	}
	return nil;
}

- (void)tableView:(UITableView *) tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	UITableViewCell* selectedCell = [tableView cellForRowAtIndexPath:indexPath];
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	editor.editable = NO;
	editorButton.enabled = NO;
	switch (indexPath.section) {
		case 0:
			if ([configFiles count] == 0) return;
			if (configIndexPath) {
				[tableView cellForRowAtIndexPath:configIndexPath].accessoryType = UITableViewCellAccessoryNone;
			}
			selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
			selectedCell.highlighted = NO;

			self.configIndexPath = indexPath;
			self.editor.text = [NSString stringWithContentsOfFile:[self selectedConfigPath] encoding:NSUTF8StringEncoding error:nil];
			editor.editable = YES;
			editorButton.enabled = YES;
			if (editorVC != nil){
				editorVC.navigationItem.rightBarButtonItem = editorButton;
				[editorVC setTitle: [configFiles objectAtIndex:configIndexPath.row]];
				[rootVC pushViewController:editorVC animated:YES];
			}
			
			break;
		case 1:
			if ([installFiles count] == 0) return;
			[self performSelectorInBackground:@selector(installGame:) withObject:indexPath];
			break;
		case 2:
			self.editor.text = [NSString stringWithContentsOfFile:[docDir stringByAppendingFormat:@"/%@", [logFiles objectAtIndex:indexPath.row]] encoding:NSUTF8StringEncoding error:nil];
			if (editorVC != nil){
				editorVC.navigationItem.rightBarButtonItem = editorButton;
				[editorVC setTitle: [configFiles objectAtIndex:configIndexPath.row]];
				[rootVC pushViewController:editorVC animated:YES];
			}
			break;
	}
}

- (void)tableView:(UITableView *) __unused tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (editingStyle == UITableViewCellEditingStyleInsert) return;
	
	NSFileManager* fm = [NSFileManager defaultManager];
	switch (indexPath.section) {
		case 0:
			[fm removeItemAtPath:[NSString stringWithFormat:@"%@/%@", docDir, [configFiles objectAtIndex:indexPath.row]] error:nil];
			break;
		case 1:
			[fm removeItemAtPath:[NSString stringWithFormat:@"%@/%@", docDir, [installFiles objectAtIndex:indexPath.row]] error:nil];
			break;
		case 2:
			[fm removeItemAtPath:[NSString stringWithFormat:@"%@/%@", docDir, [logFiles objectAtIndex:indexPath.row]] error:nil];
			break;
	}
	[self reloadTableData];
}

- (BOOL)tableView:(UITableView *) __unused tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
	switch (indexPath.section) {
		case 0:
			return (BOOL)[configFiles count];
		case 1:
			return (BOOL)[installFiles count];
		case 2:
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
	finished = YES;
	CFRunLoopStop(CFRunLoopGetCurrent());
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
