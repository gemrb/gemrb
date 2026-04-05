// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#import <QuartzCore/QuartzCore.h>
#import <SDL.h>
#import <UIKit/UIKit.h>

extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
#include <SDL/SDL_keyboard.h>

#pragma mark UIKit button subclasses
@interface GEM_UIKit_RoundedButton : UIButton
{}
@end

@implementation GEM_UIKit_RoundedButton

- (id)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self) {
		//custom drawing
		self.backgroundColor = [UIColor colorWithRed:0.196 green:0.3098 blue:0.52 alpha:1.0];
		self.titleLabel.textColor = [UIColor darkTextColor];
		self.titleLabel.font = [UIFont systemFontOfSize:24];
		self.layer.cornerRadius = 10;
		self.layer.borderWidth = 2;
		self.layer.borderColor = [UIColor grayColor].CGColor;
		self.clipsToBounds = YES;
	}
	return self;
}

@end

@interface SDL_UIKit_ModifierKeyButton : GEM_UIKit_RoundedButton {
@private
    SDL_Keymod _modifierKey;
}
@property(nonatomic, assign, getter = modifierKey, setter = setModifierKey:) SDL_Keymod _modifierKey;

- (void)toggleModifier;
@end

@implementation SDL_UIKit_ModifierKeyButton
@synthesize _modifierKey;

- (id)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self) {
		_modifierKey = KMOD_NONE;
		[self setSelected:NO];
		[self addTarget:self action:@selector(toggleModifier) forControlEvents:UIControlEventTouchDown];
	}
	return self;
}

- (BOOL)selected
{
	return (BOOL)(SDL_GetModState() & _modifierKey);
}

- (void)setSelected:(BOOL)selected
{
	[super setSelected:selected];
	if (selected){
		SDL_SetModState(SDL_GetModState() | _modifierKey);
		self.backgroundColor = [UIColor colorWithRed:0.196 green:0.3098 blue:0.52 alpha:1.0];
	}else{
		SDL_SetModState(SDL_GetModState() & ~_modifierKey);
		self.backgroundColor = [UIColor colorWithRed:0.196 green:0.3098 blue:0.52 alpha:0.25];
	}
}

- (void)toggleModifier
{
	[self setSelected:!self.selected];
}

@end

@interface SDL_UIKit_KeyButton : GEM_UIKit_RoundedButton {
@private
    SDL_Scancode _keyCode;
}
@property(nonatomic, assign, getter = keyCode, setter = setKeyCode:) SDL_Scancode _keyCode;

- (void)sendKey;
@end

@implementation SDL_UIKit_KeyButton
@synthesize _keyCode;

- (id)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self) {
		_keyCode = SDL_SCANCODE_UNKNOWN;
		[self addTarget:self action:@selector(sendKey) forControlEvents:UIControlEventTouchDown];
	}
	return self;
}

- (void)sendKey
{
	//send both press and release
	SDL_SendKeyboardKey(SDL_PRESSED, _keyCode);
	SDL_SendKeyboardKey(SDL_RELEASED, _keyCode);
}

@end

#pragma mark end button subclasses
// need a custom accessory view for the keybaord (for ctrl key modifier)
@interface SDL_UIKIT_KBoardAccessoryView : UIView
{}
- (void)willMoveToWindow:(UIWindow *)newWindow;
@end

@implementation SDL_UIKIT_KBoardAccessoryView

- (id) initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self){
		self.backgroundColor = [UIColor colorWithRed:138.0/255.0 green:139.0/255.0 blue:147.0/255.0 alpha:1.0];
	}
	return self;
}

- (void)willMoveToWindow:(UIWindow *)newWindow
{
	// newWindow is the window with the soft keybord
	[super willMoveToWindow:newWindow];
	newWindow.alpha = 0.60;
}

@end

@interface UITextField (KeyboardAccesory)
-(UIView *)inputAccessoryView;// override UIKit and force our own
@end

@implementation UITextField (KeyboardAccesory)
- (UIView *)inputAccessoryView
{
	static SDL_UIKIT_KBoardAccessoryView* accessoryView = nil;
	if (!accessoryView){
		CGRect accessFrame = CGRectMake(0.0, 0.0, [UIScreen mainScreen].bounds.size.width, 77.0);
        accessoryView = [[SDL_UIKIT_KBoardAccessoryView alloc] initWithFrame:accessFrame];
		
		CGFloat xSpacing = 20.0;
		CGFloat width = 120.0;
		CGFloat xPos = xSpacing;
		//ctrl key
        SDL_UIKit_ModifierKeyButton *ctrlButton = [[SDL_UIKit_ModifierKeyButton alloc] initWithFrame:CGRectMake(xPos, 20.0, width, 40.0)];
		ctrlButton.modifierKey = KMOD_CTRL;
        [ctrlButton setTitle: @"Ctrl" forState:UIControlStateNormal];
        [accessoryView addSubview:ctrlButton];
		[ctrlButton release];
		//alt key
		xPos += xSpacing + width;
		SDL_UIKit_ModifierKeyButton *altButton = [[SDL_UIKit_ModifierKeyButton alloc] initWithFrame:CGRectMake(xPos, 20.0, width, 40.0)];
		altButton.modifierKey = KMOD_ALT;
        [altButton setTitle: @"Alt" forState:UIControlStateNormal];
        [accessoryView addSubview:altButton];
		[altButton release];
		//pgUp key
		xPos += xSpacing + width;
		SDL_UIKit_KeyButton *pgKey = [[SDL_UIKit_KeyButton alloc] initWithFrame:CGRectMake(xPos, 20.0, width, 40.0)];
		pgKey.keyCode = SDL_SCANCODE_PAGEUP;
        [pgKey setTitle: @"PgUp" forState:UIControlStateNormal];
        [accessoryView addSubview:pgKey];
		[pgKey release];
		//pgDown key
		xPos += xSpacing + width;
		pgKey = [[SDL_UIKit_KeyButton alloc] initWithFrame:CGRectMake(xPos, 20.0, width, 40.0)];
		pgKey.keyCode = SDL_SCANCODE_PAGEDOWN;
        [pgKey setTitle: @"PgDown" forState:UIControlStateNormal];
        [accessoryView addSubview:pgKey];
		[pgKey release];
	}
    return accessoryView;
}
@end
