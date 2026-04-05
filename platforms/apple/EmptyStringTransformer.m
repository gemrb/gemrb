// SPDX-FileCopyrightText: 2013 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#import "EmptyStringTransformer.h"

@implementation EmptyStringTransformer

+(Class)transformedValueClass {
    return [NSString class];
}

-(id)transformedValue:(id)value {
	if ([value isKindOfClass:[NSString class]]) {
		return value;
	}
    if (value == nil) {
        return @"";
    }
	return nil;
}

@end
