/* hash.c - DHCP server hash table routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,09oct01,vvv  fixed mod hist
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library contains the code used by the DHCP server to add and remove 
entries from the internal hash tables. The following hash tables are used:

     cidhashtable - contains resource entries accessed by client ID.
                    (These entries are by definition for infinite leases
                     since they may only be issued to a specific client).

     iphashtable - contains resource entries accessed by IP address, so that
                   client requests for particular IP addresses may be 
                   processed efficiently.

     nmhashtable - contains resource entries accessed by entry name. Used
                   when reading binding database from permanent storage and
                   when quoting previously defined address pool entries.

     relayhashtable - contains list of known relay agents, accessed by 
                      IP address.

     paramhashtable - contains client-specific and class-specific DHCP options 
                      accessed by corresponding identifier.

INCLUDE_FILES: None
*/

/*
 * Modified by Akihiro Tominaga. (tomy@sfc.wide.ad.jp)
 */
/*
 * Generalized hash table ADT
 *
 * Provides multiple, dynamically-allocated, variable-sized hash tables on
 * various data and keys.
 *
 * This package attempts to follow some of the coding conventions suggested
 * by Bob Sidebotham and the AFS Clean Code Committee of the
 * Information Technology Center at Carnegie Mellon.
 *
 *
 *
 * Copyright (c) 1988 by Carnegie Mellon.
 *
 * Permission to use, copy, modify, and distribute this program for any
 * purpose and without fee is hereby granted, provided that this copyright
 * and permission notice appear on all copies and supporting documentation,
 * the name of Carnegie Mellon not be used in advertising or publicity
 * pertaining to distribution of the program without specific prior
 * permission, and notice be given in supporting documentation that copying
 * and distribution is by permission of Carnegie Mellon.  Carnegie Mellon
 * makes no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include "vxWorks.h"

#include <stdio.h>
#include <stdlib.h>
#include "dhcp/hash.h"
static unsigned	  hash_func();

/*******************************************************************************
*
* hash_func - calculate a hash code from the given string
*
* This function returns the sum of the squares of all the bytes for use as
* the "hashcode" parameter in to other functions in this library. The size
* of all hashtables currently in use is a prime number. This algorithm
* probably works best under that condition.
*
* RETURNS: Calculated hash code.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static unsigned hash_func
    (
    char *string, 	/* string to use for hashcode */
    FAST unsigned len 	/* length of input string */
    )
    {
    FAST unsigned sum; 
    FAST unsigned value = 0;

    sum = 0;
    for (; len > 0; len--) 
        {
        value = (unsigned) (*string++ & 0xFF);
        sum += value * value;
        }
    return (sum);
    }

/*******************************************************************************
*
* hash_ins - add an element to a hash table
*
* This function inserts the given element into the specified hash table after 
* determining the correct hashcode. If the data item is already present, the
* insertion fails.
*
* RETURNS: 0 if insertion successful, or -1 on duplicate item or error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int hash_ins
    (
    struct hash_tbl *hashtable,	/* target hash table */
    char *str, 			/* string to use for hash code */
    unsigned length, 		/* length of hash code string */
    int (*compare)(), 		/* comparison function for hash keys */
    hash_datum *key, 		/* hash key for detection of duplicates */
    hash_datum *element 	/* data to store in hash table */
    )
    {
    unsigned hashcode;
    struct hash_member *memberptr = NULL; 
    struct hash_member *temp = NULL;

    /* Calculate appropriate hashcode. */

    hashcode = hash_func (str, length);
    hashcode %= HASHTBL_SIZE;

    /* Check for duplicate entry. */

    if (hash_exst (hashtable, hashcode, compare, key)) 
        return (-1); 

    /* Store new element in table. */

    memberptr = (hashtable->head) [hashcode];
    temp = (struct hash_member *)calloc (1, sizeof (struct hash_member));
    if (temp == NULL) 
        return (-1);  
    else
        {
        temp->data = element;
        temp->next = memberptr;
        (hashtable->head) [hashcode] = temp;
        }

    return(0);
    }

/*******************************************************************************
*
* hash_exst - check for existing hash table entries
*
* This routine uses the provided comparison function to determine if 
* a given data item is already present in the indicated bucket of the
* specified hash table. 
*
* RETURNS: TRUE if matching entry found, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int hash_exst
    (
    struct hash_tbl *hashtable,		/* target hash table */
    unsigned hashcode, 			/* offset in table */
    int (*compare)(), 			/* comparison function for hash keys */
    hash_datum *key 			/* key to detect duplicate entries */
    )
    {
    FAST struct hash_member *memberptr = NULL;

    memberptr = (hashtable->head) [hashcode % HASHTBL_SIZE];
    while (memberptr) 
        {
        if ( (*compare) (key, memberptr->data)) 
            return TRUE;    /* Entry does exist */
        memberptr = memberptr->next;
        }
    return FALSE;    /* Entry does not exist */
    }

/*******************************************************************************
*
* hash_find - return a hash table entry
*
* This function locates the data entry associated with the given key, if any.
*
* RETURNS: Pointer to data entry, or NULL if not found.
*
* ERRNO: N/A
*
* NOMANUAL
*/

hash_datum * hash_find
    (
    struct hash_tbl *hashtable,	/* target hash table */
    char *str, 			/* string to use for hash code */
    unsigned length, 		/* length of hash code string */
    int (*compare)(), 		/* comparison function for hash keys */
    hash_datum *key 		/* hash key for desired entry */
    )
    {
    unsigned hashcode;
    struct hash_member *memberptr = NULL;

    hashcode = hash_func (str, length);
    memberptr = (hashtable->head) [hashcode % HASHTBL_SIZE];
    while (memberptr) 
        {
        if ( (*compare) (key, memberptr->data)) 
            return (memberptr->data);
        memberptr = memberptr->next;
        }
    return NULL;
    }

/*******************************************************************************
*
* hash_pickup - find and remove a hash table entry
*
* Like hash_exst(), this function searches the indicated bucket of the 
* specified hash table for a data entry associated with the given key. 
* If found, the matching entry is removed from the hash table.
* of all hashtables currently in use is a prime number. This algorithm
* probably works best under that condition.
*
* RETURNS: Pointer to extracted entry, or NULL if not found.
*
* ERRNO: N/A
*
* NOMANUAL
*/

hash_datum * hash_pickup
    (
    struct hash_tbl *hashtable, 	/* target hash table */
    unsigned hashcode, 			/* offset in hash table */
    int (*compare) (), 			/* comparison function for hash keys */
    hash_datum *key 			/* hash key for desired entry */
    )
    {
    struct hash_member *memberptr = NULL;
    struct hash_member *previous = NULL;
    hash_datum *result = NULL;

    memberptr = (hashtable->head) [hashcode % HASHTBL_SIZE];
    while (memberptr) 
        {
        if ( (*compare) (key, memberptr->data)) 
            {
            if (memberptr == *hashtable->head) 
	        *hashtable->head = memberptr->next;
            else 
	        previous->next = memberptr->next;
            result = memberptr->data;
            free (memberptr);
            return (result);
            }
        previous = memberptr;
        memberptr = memberptr->next;
        }
    return (NULL);
    }

/*******************************************************************************
*
* hash_del - remove hash table elements
*
* This routine deletes all data elements which match the given key.
*
* RETURNS: 0 if deletion successful, or -1 if no matching elements found.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int hash_del
    (
    struct hash_tbl *hashtable, 	/* hash table to be cleaned */
    char *str, 				/* string to use for hash code */
    unsigned length, 			/* length of hash code string */
    int (*compare) (), 			/* comparison function for hash keys */
    hash_datum *key, 			/* hash key of target entries */
    int (*free_data) () 		/* function to remove table entry */
    )
    {
    unsigned hashcode;
    struct hash_member *memberptr = NULL;
    struct hash_member *previous = NULL;
    struct hash_member *tempptr = NULL;

    int retval = -1;

    hashcode = hash_func (str, length);
    hashcode %= HASHTBL_SIZE;

    /* Remove matching entries at the head of the list. */

    memberptr = (hashtable->head) [hashcode];
    while (memberptr != NULL && (*compare) (key, memberptr->data)) 
        {
        (hashtable->head) [hashcode] = memberptr->next;
   
         /*
          * Clear next pointer to prevent list traversal by removal function. 
          */

        memberptr->next = NULL;
        free_data (memberptr);
        memberptr = (hashtable->head) [hashcode];
        retval = 0;
        }

    /*
     * Now traverse the rest of the list.
     */

    previous = memberptr;
    if (memberptr != NULL) 
        memberptr = memberptr->next;

    while (memberptr != NULL) 
        {
        if ( (*compare) (key, memberptr->data)) 
            {
            tempptr = memberptr;

            /* Bypass lease entry to be removed. */

            previous->next = memberptr = memberptr->next;

            /*
             * Prevent list traversal by removal function. 
             */

            tempptr->next = NULL;
            free_data (tempptr);
            retval = 0;
            } 
        else 
            {
            previous = memberptr;
            memberptr = memberptr->next;
            }
        }
    return retval;
    }
