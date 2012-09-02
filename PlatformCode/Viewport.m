//
//  Viewport.m
//  Brogue
//
//  Created by Brian and Kevin Walker on 11/28/08.
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

#import "Viewport.h"

@implementation Viewport

NSSize characterSize;

short vPixels = 18;
short hPixels = 9;

short theFontSize = 13;

- (id)initWithFrame:(NSRect)rect
{
	if ( ![super initWithFrame:rect] ) {
		return nil;
	}
	
	int i, j;
	
	for ( j = 0; j < kROWS; j++ ) {
		for ( i = 0; i < kCOLS; i++ ) {
			letterArray[i][j] = @" ";
			[letterArray[i][j] retain];
			bgColorArray[i][j] = [[NSColor whiteColor] retain];
			
			attributes[i][j] = [[NSMutableDictionary alloc] init];
			[attributes[i][j] setObject:[NSFont fontWithName: @"Monaco" size: theFontSize]
						   forKey:NSFontAttributeName];
			[attributes[i][j] setObject:[NSColor blackColor]
						   forKey:NSForegroundColorAttributeName];			
			rectArray[i][j] = NSMakeRect(HORIZ_PX*i, (VERT_PX * kROWS)-(VERT_PX*(j+1)), HORIZ_PX, VERT_PX);
		}
	}
	
	characterSizeDictionary = [[NSMutableDictionary dictionaryWithCapacity:100] retain];
	
	characterSize = [@"a" sizeWithAttributes:attributes[0][0]]; // no need to do this every time we draw a character
	
	//NSLog(@"in initWithFrame, rect is (%f, %f)", rect.origin.x, rect.origin.y	);
	 
	return self;
}

- (BOOL)isOpaque
{
	return YES;
}

- (void)setString:(NSString *)c 
   withBackground:(NSColor *)bgColor
  withLetterColor:(NSColor *)letterColor
	  atLocationX:(short)x
		locationY:(short)y
{
	NSRect updateRect;
	NSSize stringSize;
		
	[letterArray[x][y] release];
	[bgColorArray[x][y] release];
	letterArray[x][y] = [c retain];
	bgColorArray[x][y] = [bgColor retain];
	[attributes[x][y] setObject:letterColor forKey:NSForegroundColorAttributeName];
	
	//[self setNeedsDisplayInRect:rectArray[x][y]];
	
	stringSize = [[characterSizeDictionary objectForKey:c] sizeValue];
	stringSize.width += 1;
	
	if (stringSize.width >= rectArray[x][y].size.width) { // custom update rectangle
		updateRect.origin.y = rectArray[x][y].origin.y;
		updateRect.size.height = rectArray[x][y].size.height;
		updateRect.origin.x = rectArray[x][y].origin.x + (rectArray[x][y].size.width - stringSize.width - 10)/2;
		updateRect.size.width = stringSize.width + 10;
		[self setNeedsDisplayInRect:updateRect];
	} else { // fits within the cell rectangle; no need for a custom update rectangle
		[self setNeedsDisplayInRect:rectArray[x][y]];
		//[self setNeedsDisplay:TRUE];
	}
}

- (void)drawRect:(NSRect)rect
{
	int i, j, startX, startY, endX, endY;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	startX = (int) (rect.origin.x / hPixels);
	startY = kROWS - (int) ((rect.origin.y + rect.size.height + vPixels - 1 ) / vPixels);
	endX = (rect.origin.x + rect.size.width + hPixels - 1) / hPixels;
	endY = kROWS - (int) (rect.origin.y / vPixels);
	if (startX < 0) {
		startX = 0;
	}
	if (endX >= kCOLS) {
		endX = kCOLS;
	}
	if (startY < 0) {
		startY = 0;
	}
	if (endY >= kROWS) {
		endY = kROWS;
	}
	for ( j = startY; j < endY; j++ ) {
		for ( i = startX; i < endX; i++ ) {
			[bgColorArray[i][j] set];
			[NSBezierPath fillRect:rectArray[i][j]];
			//NSLog(@"bgColorArray[%i][%i] is %@; letter is %@, letter color is %@", i, j, bgColorArray[i][j], letterArray[i][j], [attributes[i][j] objectForKey:NSForegroundColorAttributeName]);
			[self drawTheString:letterArray[i][j] centeredIn:rectArray[i][j] withAttributes:attributes[i][j]];
		}
	}
	[pool drain];
}

- (void)drawTheString:(NSString *)theString centeredIn:(NSRect)rect withAttributes:(NSMutableDictionary *)theAttributes
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSPoint stringOrigin;
	NSSize stringSize;
	
	//	NSLog(@"theString is '%@'", theString);
	
	if ( theString == nil ) {
		return;
	}
	
	if ( [characterSizeDictionary objectForKey:theString] == nil ) {
		stringSize = [theString sizeWithAttributes:theAttributes];	// quite expensive
		//	NSLog(@"stringSize for '%@' has width %f and height %f", theString, stringSize.width, stringSize.height);
		[characterSizeDictionary setObject:[NSValue valueWithSize:stringSize] forKey:theString];
		
	} else {
		stringSize = [[characterSizeDictionary objectForKey:theString] sizeValue];
	}

	stringOrigin.x = rect.origin.x + (rect.size.width - stringSize.width)/2;
	stringOrigin.y = rect.origin.y + (rect.size.height - stringSize.height)/2;

	[theString drawAtPoint:stringOrigin withAttributes:theAttributes];
	
	[pool drain];
}

- (short)horizPixels
{
	return hPixels;
}

- (short)vertPixels
{
	return vPixels;
}

- (short)fontSize
{
	return theFontSize;
}

- (void)setHorizPixels:(short)hPx
		   vertPixels:(short)vPx
			 fontSize:(short)size
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	int i, j;
	hPixels = hPx;
	vPixels = vPx;
	theFontSize = size;
	
	for ( j = 0; j < kROWS; j++ ) {
		for ( i = 0; i < kCOLS; i++ ) {
			[attributes[i][j] setObject:[NSFont fontWithName: @"Monaco" size:theFontSize]
			 forKey:NSFontAttributeName];
			rectArray[i][j] = NSMakeRect(hPixels*i, (vPixels * kROWS)-(vPixels*(j+1)), hPixels, vPixels);
		}
	}
	characterSize = [@"a" sizeWithAttributes:attributes[0][0]];
	
	[characterSizeDictionary removeAllObjects];
	
	[pool drain];
}

@end
