// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#import <UIKit/UIKit.h>

@protocol GEM_ConfControllerDelegate
- (void)setupComplete:(NSString*)configPath;
@end

@interface GEM_NavController : UINavigationController {
@private
}
	@end

	@interface GEM_ConfController : NSObject <UINavigationControllerDelegate> {
	@private

		NSString* docDir;
		NSArray* configFiles;
		NSArray* installFiles;
		NSArray* installedGames;
		NSArray* logFiles;

		NSArray* toolBarItems;

		NSIndexPath* configIndexPath;

		UITextView* editor;
		UITableView* controlTable;

		UINavigationController* rootVC;
		UIViewController* editorVC;

		UIBarButtonItem* editorButton;
		UIBarButtonItem* playButton;
	}
	@property(nonatomic, retain) IBOutlet UITableView* controlTable;
	@property(nonatomic, retain) NSIndexPath* configIndexPath;
	@property(nonatomic, retain) IBOutlet UITextView* editor;
	@property(nonatomic, retain) IBOutlet UINavigationController* rootVC;
	@property(nonatomic, retain) IBOutlet UIViewController* editorVC;
	@property(nonatomic, retain) IBOutlet UIBarButtonItem* playButton;
	@property(nonatomic, retain) id delegate;

	- (void)reloadTableData;
	- (NSString*)selectedConfigPath;
	- (BOOL)installGame:(NSIndexPath*)indexPath;

	- (NSInteger)numberOfSectionsInTableView:(UITableView*)tableView;
	- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath;
	- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section;
	- (NSString*)tableView:(UITableView*)tableView titleForHeaderInSection:(NSInteger)section;
	- (void)tableView:(UITableView*)tableView didDeselectRowAtIndexPath:(NSIndexPath*)indexPath;
	- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath;

	- (IBAction)saveConfig:(id)sender;
	- (IBAction)launchGEM:(id)sender;

	- (void)navigationController:(UINavigationController*)navigationController willShowViewController:(UIViewController*)viewController animated:(BOOL)animated;
	@end
