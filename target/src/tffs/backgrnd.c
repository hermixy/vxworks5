
/*
 * $Log:   P:/user/amir/lite/vcs/backgrnd.c_v  $
 * 
 *    Rev 1.8   19 Aug 1997 20:09:16   danig
 * Andray's changes
 * 
 *    Rev 1.7   27 Jul 1997 14:30:12   danig
 * "fl" prefix, flStart\EndCriticalSection
 * 
 *    Rev 1.6   07 Jul 1997 15:20:56   amirban
 * Ver 2.0
 * 
 *    Rev 1.5   01 May 1997 12:15:40   amirban
 * Fixed drive switching bug
 * 
 *    Rev 1.4   03 Nov 1996 17:39:58   amirban
 * Call map instead of setMappingContext
 * 
 *    Rev 1.3   29 Aug 1996 14:16:34   amirban
 * Add Win32 option
 * 
 *    Rev 1.2   18 Aug 1996 13:48:54   amirban
 * Comments
 * 
 *    Rev 1.1   12 Aug 1996 15:32:30   amirban
 * Handle drive changes and context saving
 * 
 *    Rev 1.0   31 Jul 1996 14:31:58   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

#include "flsocket.h"
#include "backgrnd.h"

#ifdef BACKGROUND

#include <setjmp.h>
#include <dos.h>

static void (*backgroundTask)(void *);

typedef struct {
  jmp_buf	registers;
  unsigned	mappingContext;
  void *	object;
  FLSocket *	socket;
} Context;

static Context foregroundContext, backgroundContext, *currentContext;

static int switchContext(Context *toContext, int sendValue)
{
  int state;

  if (toContext == currentContext)
    return 0;

  state = setjmp(currentContext->registers);	/* save our state */
  if (state == 0) {
    if (backgroundContext.socket) {
      currentContext->mappingContext = flGetMappingContext(backgroundContext.socket);
      if (toContext->mappingContext != UNDEFINED_MAPPING)
	flMap(backgroundContext.socket,(CardAddress) toContext->mappingContext << 12);
    }
    currentContext = toContext;
    longjmp(toContext->registers,sendValue);
  }
  /* We are back here when target task suspends, and 'state'
     is the 'sendValue' on suspend */
  return state;
}


int flForeground(int sendValue)
{
  return switchContext(&foregroundContext,sendValue);
}


int flBackground(int sendValue)
{
  return switchContext(&backgroundContext,sendValue);
}


static char backgroundStack[200];

int flStartBackground(unsigned volNo, void (*routine)(void *), void *object)
{
  if (currentContext != &foregroundContext)
    return 0;

  while (backgroundTask)		/* already exists */
    flBackground(BG_RESUME);		/* run it until it finishes */
  backgroundTask = routine;
  backgroundContext.object = object;
  backgroundContext.socket = flSocketOf(volNo);
  flNeedVcc(backgroundContext.socket);

  return flBackground(BG_RESUME);
}


void flCreateBackground(void)
{
  FLMutex dummyMutex;

  foregroundContext.socket = backgroundContext.socket = NULL;

  if (setjmp(foregroundContext.registers) != 0)
    return;

#ifdef __WIN32__
   _ESP = (void *) (backgroundStack + sizeof backgroundStack);
#else
  flStartCriticalSection(&dummyMutex);
  _SP = FP_OFF((void far *) (backgroundStack + sizeof backgroundStack));
  _SS = FP_SEG((void far *) (backgroundStack + sizeof backgroundStack));
  flEndCriticalSection(&dummyMutex);
#endif
  backgroundTask = NULL;
  if (setjmp(backgroundContext.registers) == 0) {
    currentContext = &foregroundContext;
    longjmp(foregroundContext.registers,1);       /* restore stack and continue */
  }

  /* We are back here with our new stack when 'background' is called */
  for (;;) {
    if (backgroundTask) {
      (*backgroundTask)(backgroundContext.object);
      flDontNeedVcc(backgroundContext.socket);
      backgroundTask = NULL;
    }
    flForeground(-1);
  }
}

#endif


