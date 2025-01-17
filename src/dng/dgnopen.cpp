/******************************************************************************
 * $Id: dgnopen.cpp,v 1.17 2003/06/12 17:33:17 warmerda Exp $
 *
 * Project:  Microstation DGN Access Library
 * Purpose:  DGN Access Library file open code.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Avenza Systems Inc, http://www.avenza.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log: dgnopen.cpp,v $
 * Revision 1.17  2003/06/12 17:33:17  warmerda
 * improved DNGO_CAPTURE_RAW_DATA flag documnentation
 *
 * Revision 1.16  2003/05/21 03:42:01  warmerda
 * Expanded tabs
 *
 * Revision 1.15  2003/05/12 18:48:57  warmerda
 * added preliminary 3D write support
 *
 * Revision 1.14  2002/04/22 20:44:41  warmerda
 * added (partial) cell library support
 *
 * Revision 1.13  2002/03/14 21:39:27  warmerda
 * added bUpdate to DGNOpen()
 *
 * Revision 1.12  2002/02/22 22:17:42  warmerda
 * Ensure that components of complex chain/shapes are spatially selected
 * based on the decision made for their owner (header).
 *
 * Revision 1.11  2002/01/21 20:52:11  warmerda
 * added SetSpatialFilter function
 *
 * Revision 1.10  2001/09/27 14:28:44  warmerda
 * first hack at 3D support
 *
 * Revision 1.9  2001/08/21 03:01:39  warmerda
 * added raw_data support
 *
 * Revision 1.8  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.7  2001/06/27 16:14:21  warmerda
 * Free the element index on close (patch c/o Tom Parker, avs.com).
 *
 * Revision 1.6  2001/03/18 16:54:39  warmerda
 * added use of DGNTestOpen, remove extention test
 *
 * Revision 1.5  2001/03/07 13:56:44  warmerda
 * updated copyright to be held by Avenza Systems
 *
 * Revision 1.4  2001/01/10 16:13:09  warmerda
 * added documentation
 *
 * Revision 1.3  2000/12/28 21:28:59  warmerda
 * added element index support
 *
 * Revision 1.2  2000/12/14 17:10:57  warmerda
 * implemented TCB, Ellipse, TEXT
 *
 * Revision 1.1  2000/11/28 19:03:47  warmerda
 * New
 *
 */

#include "dgnlibp.h"

CPL_CVSID("$Id: dgnopen.cpp,v 1.17 2003/06/12 17:33:17 warmerda Exp $");

/************************************************************************/
/*                            DGNTestOpen()                             */
/************************************************************************/

/**
 * Test if header is DGN.
 *
 * @param pabyHeader block of header data from beginning of file.
 * @param nByteCount number of bytes in pabyHeader.
 *
 * @return TRUE if the header appears to be from a DGN file, otherwise FALSE.
 */

int DGNTestOpen( GByte *pabyHeader, int nByteCount )
{
    if( nByteCount < 4 )
        return TRUE;

    // Is it a cell library?
    if( pabyHeader[0] == 0x08
            && pabyHeader[1] == 0x05
            && pabyHeader[2] == 0x17
            && pabyHeader[3] == 0x00 )
        return TRUE;

    // Is it not a regular 2D or 3D file?
    if( (pabyHeader[0] != 0x08 && pabyHeader[0] != 0xC8)
            || pabyHeader[1] != 0x09
            || pabyHeader[2] != 0xFE || pabyHeader[3] != 0x02 )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                              DGNOpen()                               */
/************************************************************************/

/**
 * Open a DGN file.
 *
 * The file is opened, and minimally verified to ensure it is a DGN (ISFF)
 * file.  If the file cannot be opened for read access an error with code
 * CPLE_OpenFailed with be reported via CPLError() and NULL returned.
 * If the file header does
 * not appear to be a DGN file, an error with code CPLE_AppDefined will be
 * reported via CPLError(), and NULL returned.
 *
 * If successful a handle for further access is returned.  This should be
 * closed with DGNClose() when no longer needed.
 *
 * DGNOpen() does not scan the file on open, and should be very fast even for
 * large files.
 *
 * @param pszFilename name of file to try opening.
 * @param bUpdate should the file be opened with read+update (r+) mode?
 *
 * @return handle to use for further access to file using DGN API, or NULL
 * if open fails.
 */

DGNHandle DGNOpen( const char * pszFilename, int bUpdate )
{
    DGNInfo     *psDGN;
    FILE        *fp;

    /* -------------------------------------------------------------------- */
    /*      Open the file.                                                  */
    /* -------------------------------------------------------------------- */
    if( bUpdate )
        fp = VSIFOpen( pszFilename, "rb+" );
    else
        fp = VSIFOpen( pszFilename, "rb" );
    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to open `%s' for read access.\n",
                  pszFilename );
        return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*      Verify the format ... add later.                                */
    /* -------------------------------------------------------------------- */
    GByte       abyHeader[512];

    VSIFRead( abyHeader, 1, sizeof(abyHeader), fp );
    if( !DGNTestOpen( abyHeader, sizeof(abyHeader) ) )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "File `%s' does not have expected DGN header.\n",
                  pszFilename );
        VSIFClose( fp );
        return NULL;
    }

    VSIRewind( fp );

    /* -------------------------------------------------------------------- */
    /*      Create the info structure.                                      */
    /* -------------------------------------------------------------------- */
    psDGN = (DGNInfo *) CPLCalloc(sizeof(DGNInfo),1);
    psDGN->fp = fp;
    psDGN->next_element_id = 0;

    psDGN->got_tcb = FALSE;
    psDGN->scale = 1.0;
    psDGN->origin_x = 0.0;
    psDGN->origin_y = 0.0;
    psDGN->origin_z = 0.0;

    psDGN->index_built = FALSE;
    psDGN->element_count = 0;
    psDGN->element_index = NULL;

    psDGN->got_bounds = FALSE;

    if( abyHeader[0] == 0xC8 )
        psDGN->dimension = 3;
    else
        psDGN->dimension = 2;

    psDGN->has_spatial_filter = FALSE;
    psDGN->sf_converted_to_uor = FALSE;
    psDGN->select_complex_group = FALSE;
    psDGN->in_complex_group = FALSE;

    return (DGNHandle) psDGN;
}

/************************************************************************/
/*                           DGNSetOptions()                            */
/************************************************************************/

/**
 * Set file access options.
 *
 * Sets a flag affecting how the file is accessed.  Currently
 * there is only one support flag:
 *
 * DGNO_CAPTURE_RAW_DATA: If this is enabled (it is off by default),
 * then the raw binary data associated with elements will be kept in
 * the raw_data field within the DGNElemCore when they are read.  This
 * is required if the application needs to interprete the raw data itself.
 * It is also necessary if the element is to be written back to this file,
 * or another file using DGNWriteElement().  Off by default (to conserve
 * memory).
 *
 * @param hDGN handle to file returned by DGNOpen().
 * @param nOptions ORed option flags.
 */

void DGNSetOptions( DGNHandle hDGN, int nOptions )
{
    DGNInfo     *psDGN = (DGNInfo *) hDGN;

    psDGN->options = nOptions;
}

/************************************************************************/
/*                        DGNSetSpatialFilter()                         */
/************************************************************************/

/**
 * Set rectangle for which features are desired.
 *
 * If a spatial filter is set with this function, DGNReadElement() will
 * only return spatial elements (elements with a known bounding box) and
 * only those elements for which this bounding box overlaps the requested
 * region.
 *
 * If all four values (dfXMin, dfXMax, dfYMin and dfYMax) are zero, the
 * spatial filter is disabled.   Note that installing a spatial filter
 * won't reduce the amount of data read from disk.  All elements are still
 * scanned, but the amount of processing work for elements outside the
 * spatial filter is minimized.
 *
 * @param hDGN Handle from DGNOpen() for file to update.
 * @param dfXMin minimum x coordinate for extents (georeferenced coordinates).
 * @param dfYMin minimum y coordinate for extents (georeferenced coordinates).
 * @param dfXMax maximum x coordinate for extents (georeferenced coordinates).
 * @param dfYMax maximum y coordinate for extents (georeferenced coordinates).
 */

void DGNSetSpatialFilter( DGNHandle hDGN,
                          double dfXMin, double dfYMin,
                          double dfXMax, double dfYMax )
{
    DGNInfo     *psDGN = (DGNInfo *) hDGN;

    if( dfXMin == 0.0 && dfXMax == 0.0
            && dfYMin == 0.0 && dfYMax == 0.0 )
    {
        psDGN->has_spatial_filter = FALSE;
        return;
    }

    psDGN->has_spatial_filter = TRUE;
    psDGN->sf_converted_to_uor = FALSE;

    psDGN->sf_min_x_geo = dfXMin;
    psDGN->sf_min_y_geo = dfYMin;
    psDGN->sf_max_x_geo = dfXMax;
    psDGN->sf_max_y_geo = dfYMax;

    DGNSpatialFilterToUOR( psDGN );

}

/************************************************************************/
/*                       DGNSpatialFilterToUOR()                        */
/************************************************************************/

void DGNSpatialFilterToUOR( DGNInfo *psDGN )
{
    DGNPoint    sMin, sMax;

    if( psDGN->sf_converted_to_uor
            || !psDGN->has_spatial_filter
            || !psDGN->got_tcb )
        return;

    sMin.x = psDGN->sf_min_x_geo;
    sMin.y = psDGN->sf_min_y_geo;
    sMin.z = 0;

    sMax.x = psDGN->sf_max_x_geo;
    sMax.y = psDGN->sf_max_y_geo;
    sMax.z = 0;

    DGNInverseTransformPoint( psDGN, &sMin );
    DGNInverseTransformPoint( psDGN, &sMax );

    psDGN->sf_min_x = (GUInt32) (sMin.x + 2147483648.0);
    psDGN->sf_min_y = (GUInt32) (sMin.y + 2147483648.0);
    psDGN->sf_max_x = (GUInt32) (sMax.x + 2147483648.0);
    psDGN->sf_max_y = (GUInt32) (sMax.y + 2147483648.0);

    psDGN->sf_converted_to_uor = TRUE;
}

/************************************************************************/
/*                              DGNClose()                              */
/************************************************************************/

/**
 * Close DGN file.
 *
 * @param hDGN Handle from DGNOpen() for file to close.
 */

void DGNClose( DGNHandle hDGN )
{
    DGNInfo     *psDGN = (DGNInfo *) hDGN;

    VSIFClose( psDGN->fp );
    CPLFree( psDGN->element_index );
    CPLFree( psDGN );
}

/************************************************************************/
/*                          DGNGetDimension()                           */
/************************************************************************/

/**
 * Return 2D/3D dimension of file.
 *
 * Return 2 or 3 depending on the dimension value of the provided file.
 */

int DGNGetDimension( DGNHandle hDGN )
{
    DGNInfo     *psDGN = (DGNInfo *) hDGN;

    return psDGN->dimension;
}
