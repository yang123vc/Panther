
#import <Cocoa/Cocoa.h>
#include <vector>
#include <string>

#include "windowfunctions.h"
#include "textfile.h"
#include "controlmanager.h"

#include "renderer.h"

@class PGView;

@interface PGView : NSView <NSDraggingSource, NSWindowDelegate> {
	PGWindowHandle handle;
	PGTimerHandle timer;

	PGDropCallback dragging_callback;
	void* dragging_data;

	SkBitmap bitmap;
}

-(void)closeWindow;
-(ControlManager*)getManager;
-(PGWindowHandle)getHandle;
- (instancetype)initWithFrame:(NSRect)frameRect :(NSWindow*)window :(PGWorkspace*)workspace :(std::vector<std::shared_ptr<TextView>>)textfiles;
-(NSRect)getBounds;
- (void)targetMethod:(NSTimer*)timer;
- (BOOL)acceptsFirstMouse:(NSEvent *)event;
- (PGTimerHandle)scheduleTimer:(PGWindowHandle)handle :(int)ms :(PGTimerCallback)callback :(PGTimerFlags)flags;


- (NSDraggingSession *)startDragging:(NSArray<NSDraggingItem *> *)items 
                                    event:(NSEvent *)event 
                                    source:(id<NSDraggingSource>)source
                                    callback:(PGDropCallback)callback
                                    data:(void*)data;
-(void)draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint) screenPoint;
-(void)draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation;
-(void)draggingSession:(NSDraggingSession *)session movedToPoint:(NSPoint)screenPoint;
@end

void PGInitializeCursors();