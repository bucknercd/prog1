//mesquite room UC 2.01.24


/**********************************************************************
cs3723p1.c
Purpose:	
    This file has the function definitions for functions that allocate heap
    memory, remove references to objects, associate (create a reference) to an
    object, increment the object's ref count and print out an object
 Command Parameters:
    Description of the command parameters.  There are no commands. These
    functions are invoked by other functions
Input:
    N/A
Results:
    Depending on function called, data can be copied from the stack to the
    heap, data inside a particular function can be modfied (ref count,
    pointers), or the printing of the data of a particular node
Returns:
    all functions are void. nothing returned
Notes:
    
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs3723p1.h"

#define ULAddr(addr) ((unsigned long) addr)

/******************** smrcAllocate **********************************
void * smrcAllocate(
	StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[],
    SMResult *psmResult);
Purpose:
    allocates memory in the heap for AllocNode struct by calling smAlloc.
	sets refCount to 1, node type, and copies sbUserData from stack to
	to the heap in the correct offset of newly allocated AllocNode  
Parameters:
    I   StorageManager *pMgr	pointer,passed to smAlloc(), so it can allocate correct
    							space for new node on the heap
    I   short DataSize       	passed to smAlloc(), also use in knowing how much memory to 
    							copy from stack to the heap for node
    I   short shNodeType		used to set the node type in the newly alloc'd node
    I   char sbData             binary data in a character array, contians all data in node(on stack)
    I   SMResult *psmResult     pointer to error node , error mesg is set on this node
    O   Passed out. Value is returned through this parameter.
Notes:
    very important: userData info is being copied from the stack to our 
    virtual heap
**************************************************************************/
void * smrcAllocate(
    StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[],
    SMResult *psmResult)
{
    // pAlloc pointer. point to start of alloc node
    AllocNode *pAlloc = smAlloc(pMgr, shDataSize + 3*sizeof(short));
    if (pAlloc == NULL)
    {
        psmResult->rc = RC_INVALID_ADDR;
        strcpy(
            psmResult->szErrorMessage,
            "Invalid Address: not enough memory to alloc\n");
        return NULL;
    } 
    /* set begining sections of AllocNode and copy userDate from stack to the
     * alloc'd node (heap) */
    else 
    {
        pAlloc->shRefCount = 1;
        pAlloc->shNodeType = shNodeType;
        pAlloc->shAllocSize = shDataSize + 3*sizeof(short);
        memcpy(pAlloc->sbData, sbData, shDataSize);
        // return a pointer to the user data of this allocNode 
        return ((char *)pAlloc) + 3 * sizeof(short);
    }
}

/******************** smrcRemoveRef **********************************
void smrcRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult);
Purpose:
	lower the reference count of a particular node; if ref count reaches zero
	adjust previous associations
Parameters:
	I   StorageManager *pMgr	pointer, used to access metadata to loop thru
								different attributes; also passed to smFree()
								for error purposes (our virtual heap boundaries)

	I   void *pUserData 		pointer to the userData section of the node;
								passed to smFree so smFree knows what addr to free,
								also used to calculate our attr offsets, and to know 
								where to make an AllocNode pointer point to

	I   SMResult *psmResult		pointer to error struct, an error message may be set
								in this struct
Notes:
	a double pointer(ppNew) is used to point to another pointer at a certain 
	offset. It is needed so when we dereference it we can see what address it 
	points to. If it was just a single pointer we could't get the right information.
**************************************************************************/
void smrcRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult)
{
	int i;				// incrementer variable
	char **ppNew;   	// double pointer. used to point to ptr attribute
	char szMesg[100];	// char array for our error message
	short type;			// node type 
	AllocNode *pAlloc;	// AllocNode pointer. used to access parts of our node

	// make pAlloc point to begining of our allocNode node
    pAlloc = (AllocNode *)(((char *)pUserData) - 3*sizeof(short));
	type = pAlloc->shNodeType;

	// if the node already has zero references to it, set the error code and return
    if (pAlloc->shRefCount == 0) 
    {
    	psmResult->rc = 1;
		sprintf(
			szMesg,
			"No reference to this node. %08lX cannot be freed.\n",
                        ULAddr(pUserData));
    	strcpy(psmResult->szErrorMessage, szMesg);
    	return;
    }
    psmResult->rc = 0;

    // lower reference count of the node
    pAlloc->shRefCount--;

    // is ref count is now zero ?
    if (pAlloc->shRefCount == 0)
    {

    	// free the node and set appropriate error code (if it needs to be set)
    	smFree(pMgr, pUserData, psmResult);

    	/* starting at beginning metaAttribute subscript, iterate thru all attributes
		 *  while the node type is the correct type */
    	for(i = pMgr->nodeTypeM[type].shBeginMetaAttr;
    		pMgr->metaAttrM[i].shNodeType == type; i++)
    	{

  			// set an individual attribute as pAttr
    		MetaAttr pAttr = pMgr->metaAttrM[i];

    		// if attribute is a pointer ?
    		if (pAttr.cDataType == 'P')
    		{

    			// set the double pointer to point to the pointer in the userData 
    			ppNew = (char **)((char *)pUserData + pAttr.shOffset);
                        
                        // if the pointer's value is NULL , go to next attrib(ptr)
    			if (*ppNew == NULL)
    				continue;

                        // if pointer's val is NOT NULL, recursively call this function again
    			else 
    			    smrcRemoveRef(pMgr, *ppNew, psmResult);
    		}
    	}
    } 

    // if ref count is greater than zero, return from this func
    else if (pAlloc->shRefCount > 0) 
    {
    	return;
    }
}

/******************** smrcAssoc **********************************
void smrcAssoc(
    StorageManager *pMgr, void *pUserDataFrom, char szAttrName[],
    void *pUserDataTo, SMResult *psmResult)

Purpose:
    This function make one pointer FROM a particular node point TO 
    another node and then taking care of any other previous associations
Parameters:
    List each parameter on a separate line including data type name and 
    description.  Each item should begin with whether the parameter is passed 
    in, out or both:
    I   StorageManager *pMgr		pointer, used to access metadata to loop thru
								    different attributes; also passed to smrcRemoveRef
								    when it is called

    I   void *pUserDataFrom			pointer, points the userData of the FROM node

    I   char szAttrName[]			string, attribute name

    I   void *pUserDataTo			pointer, points to the userData of the TO node

    I   SMResult *psmResult 		pointer to error struct, an error message may be set
								    in this struct
**************************************************************************/
void smrcAssoc(
    StorageManager *pMgr, void *pUserDataFrom, char szAttrName[],
    void *pUserDataTo, SMResult *psmResult)
{
	int i;              // incrementer variable
	char **ppNew;		// double ptr. used to point to the pointer in userdata from 
    short type;
    AllocNode *pAlloc, *pToAlloc;  // 'from' and 'to' AllocNode pointers

    // make pAlloc point to the start of the 'from' userData
	pAlloc = (AllocNode *)(((char *)pUserDataFrom) - 3 * sizeof(short));

    // make pAlloc point to the start of the 'to' userData
	pToAlloc = (AllocNode *)(((char *)pUserDataTo) - 3 * sizeof(short));
	type = pAlloc->shNodeType;

	/* starting at beginning metaAttribute subscript, iterate thru all attributes
	 *  while the node type is the correct type */
	for(i = pMgr->nodeTypeM[type].shBeginMetaAttr;
    		pMgr->metaAttrM[i].shNodeType == type; i++)
	{

  		// set an individual attribute as pAttr
		MetaAttr pAttr = pMgr->metaAttrM[i];

		/* is the attrname that was passed into this func the same as the attrname of the
		 * particular attribute? if so is the attr also a pointer? */
		if ( (strcmp(szAttrName, pAttr.szAttrName) == 0 ) &&
                        pAttr.cDataType == 'P')
		{
			psmResult->rc = 0;

			// make the ppNew point to the acutal ptr in the userData section of the 'from' node
			ppNew = (char **)((char *)pUserDataFrom + pAttr.shOffset);
			if (*ppNew != NULL)
				smrcRemoveRef(pMgr, *ppNew, psmResult);

			/* copy the user data address of the node to be pointed to to the 
			 * value of the pointer in the user data */
			memcpy(ppNew, &pUserDataTo, sizeof(ppNew));
			pToAlloc->shRefCount++;
		} 

		/* is the attrname that was passed into this func NOT the same as the attrname of the
		 * particular attribute OR is the attr NOT a pointer? */
		else if ( (strcmp(szAttrName, pAttr.szAttrName) != 0 ) ||
                        pAttr.cDataType != 'P') 
		{

			// set appropriate errors
            printf("data type = %c\n", pAttr.cDataType);
			psmResult->rc = RC_ASSOC_ATTR_NOT_PTR;
			strcpy(
		               psmResult->szErrorMessage,
		               "This attribute is not a pointer.\n");	
		}
	}
}

/******************** smrcAddRef **********************************
void smrcAddRef(StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult)

Purpose:
    Increment the reference count of the particular node by one

Parameters:
    I   StorageManager *pMgr		????

    I   void *pUserDataTo			pointer, points to the userData of the TO node

    I   SMResult *psmResult			pointer to error struct, an error message may be set
								    in this struct
**************************************************************************/
void smrcAddRef(
    StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult)
{
	AllocNode *pAlloc;      // ptr to AllocNode to access AllocNode val

        // make pAlloc point to  pUserDataTo address - 6 (if short ==2 )
    pAlloc = (AllocNode *)(((char *)pUserDataTo) - 3 * sizeof(short));
	pAlloc->shRefCount++;
}

/******************** printNode **********************************
extern "C" void printNode(StorageManager *pMgr, void *pUserData) 
Purpose:
    this func goes thru a node and then prints out the values of this node
    attributes in a human readable form
Parameters:
    I   StorageManager *pMgr			pointer, used to access metadata to loop
     									thru different attributes;

    I   void *pUserData 				pointer, points to the userData portion
    									of a node
Notes:
	the ULAddr MACRO is used in this function to print out the pointers addresses
	it takes a pointer and casts it to unsigned long. Im passing into it the 
	double pointer which points to a pointer inside of the userData. But I dereference
	the double pointer to get the value inside the pointer thats in the userData so I
	do pass a single pointer to ULAddr

**************************************************************************/
extern "C" void printNode(StorageManager *pMgr, void *pUserData)
{ 

    AllocNode *pAlloc;  // ptr to AllocNode to access AllocNode vals
    char **ppPtr;       // ptr to access pointer val
    char *szPtr;        // ptr to a char array to access string val
    int *piPtr;         // ptr to an integer to access integer val
    int i;              // incrementer variable
    double *pdPtr;      // ptr to a double to access double val

    /* make pAlloc point to  pUserData address - 6 (if short ==2 ) */
    pAlloc = (AllocNode *)(((char *)pUserData) - 3 * sizeof(short));
    printf(
        "%s\t%s\t%s\t%s\t\t%s\n", "Alloc Address", "Size", "Node Type",
        "Ref Cnt", "Data Address");
    printf(
        "%08lX\t%d\t%d\t\t%d\t\t%08lX\n", ULAddr(pAlloc), pAlloc->shAllocSize,
        pAlloc->shNodeType, pAlloc->shRefCount, ULAddr(pUserData));
    printf("\n\t\t%s\t%s\t%s\n", "Attr Name", "Type", "Value");

    /* set i to the passed in nodeType's beginning attr index. For each attr
     * index, if that index's nodeType == the passed in nodeType, then
     * increment i (go thru the metaData attrs).   */
    for(i = pMgr->nodeTypeM[pAlloc->shNodeType].shBeginMetaAttr;
        pMgr->metaAttrM[i].shNodeType == pAlloc->shNodeType; i++)
    {
        MetaAttr n = pMgr->metaAttrM[i];
        
        /* check the data types. Print the data using the deferencing of 
         * their respective pointers. ( a double pointer in case of 
         * 'pointer' types */
        switch(n.cDataType)
        {
            case 'P':
                ppPtr = (char **)((char *)pUserData + n.shOffset);
                printf(
                    "\t\t%-10s\t%c\t%08lX\n", n.szAttrName, n.cDataType,
                    ULAddr(*ppPtr));
                break;
            case 'S':
                szPtr = (char *)pUserData + n.shOffset;
                printf("\t\t%-10s\t%c\t%s\n", n.szAttrName, n.cDataType, szPtr);
                break;
            case 'I':
                piPtr = (int *)((char *)pUserData + n.shOffset);
                printf(
                    "\t\t%-10s\t%c\t%d\n", n.szAttrName, n.cDataType, *piPtr);
                break;
            case 'D':
                pdPtr = (double *)((char *)pUserData + n.shOffset);
                printf(
                    "\t\t%-10s\t%c\t%f\n", n.szAttrName, n.cDataType, *pdPtr);
                break;
        }
    }
}
