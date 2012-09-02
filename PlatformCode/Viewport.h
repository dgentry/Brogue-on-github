//
//  Viewport.h
//  Brogue
//
//  Created by Brian and Kevin Walker.
//  Copyright 2012. All rights reserved.
//  
//  This file is part of Brogue.
//
//  Brogue is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Brogue is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
//

#import <Cocoa/Cocoa.h>

#define	VERT_PX		18
#define	HORIZ_PX	11

#define kROWS		(30+3+1)
#define kCOLS		100

#define FONT_SIZE	14

@interface Viewport : NSView
{
	NSString *letterArray[kCOLS][kROWS];
	NSColor *bgColorArray[kCOLS][kROWS];
	NSMutableDictionary *attributes[kCOLS][kROWS];
	NSMutableDictionary *characterSizeDictionary;
	NSRect rectArray[kCOLS][kROWS];
}

- (BOOL)isOpaque;

- (void)setString:(NSString *)c 
   withBackground:(NSColor *)bgColor
  withLetterColor:(NSColor *)letterColor
	  atLocationX:(short)x
		locationY:(short)y;

- (void)drawTheString:(NSString *)theString centeredIn:(NSRect)rect withAttributes:(NSMutableDictionary *)theAttributes;

- (short)horizPixels;
- (short)vertPixels;
- (short)fontSize;
- (void)setHorizPixels:(short)hPx
			vertPixels:(short)vPx
			  fontSize:(short)size;

@end
