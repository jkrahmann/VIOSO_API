#ifndef __SMARTPROJECTOR_SUPPORT_DELAUNAY_TRIANGLE_ESTIMATION__
#define __SMARTPROJECTOR_SUPPORT_DELAUNAY_TRIANGLE_ESTIMATION__

#pragma once
#pragma warning(disable : 4311)
#pragma warning(disable : 4312)
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)
#pragma warning(disable : 4005)

	/* For single precision (which will save some memory and reduce paging),     */
	/*   define the symbol DELAUNAY_PRECISION_SINGLE by using the -DSINGLE compiler switch or by    */
	/*   writing "#define DELAUNAY_PRECISION_SINGLE" below.                                         */
	/*                                                                           */
	/* For double precision (which will allow you to refine meshes to a smaller  */
	/*   edge length), leave DELAUNAY_PRECISION_SINGLE undefined.                                   */
	/*                                                                           */
	/* Double precision uses more memory, but improves the resolution of the     */
	/*   meshes you can generate with Triangle.  It also reduces the likelihood  */
	/*   of a floating exception due to overflow.  Finally, it is much faster    */
	/*   than single precision on 64-bit architectures like the DEC Alpha.  I    */
	/*   recommend double precision unless you want to generate a mesh for which */
	/*   you do not have enough memory.                                          */

	//#define DELAUNAY_PRECISION_SINGLE 

	#ifdef DELAUNAY_PRECISION_SINGLE
		#define DELAUNAY_REAL float
	#else /* not DELAUNAY_PRECISION_SINGLE */
		#define DELAUNAY_REAL double
	#endif /* not DELAUNAY_PRECISION_SINGLE */

	struct triangulateio
	{
		DELAUNAY_REAL *pointlist;                                      /* In / out */
		DELAUNAY_REAL *pointattributelist;                             /* In / out */
		int *pointmarkerlist;                                          /* In / out */
		int numberofpoints;                                            /* In / out */
		int numberofpointattributes;                                   /* In / out */

		int *trianglelist;                                             /* In / out */
		DELAUNAY_REAL *triangleattributelist;                          /* In / out */
		DELAUNAY_REAL *trianglearealist;                               /* In only */
		int *neighborlist;                                             /* Out only */
		int numberoftriangles;                                         /* In / out */
		int numberofcorners;                                           /* In / out */
		int numberoftriangleattributes;                                /* In / out */

		int *segmentlist;                                              /* In / out */
		int *segmentmarkerlist;                                        /* In / out */
		int numberofsegments;                                          /* In / out */

		DELAUNAY_REAL *holelist;					                   /* In / pointer to array copied out */
		int numberofholes;                                             /* In / copied out */

		DELAUNAY_REAL *regionlist;                                     /* In / pointer to array copied out */
		int numberofregions;                                           /* In / copied out */

		int *edgelist;                                                 /* Out only */
		int *edgemarkerlist;                                           /* Not used with Voronoi diagram; out only */
		DELAUNAY_REAL *normlist;                                       /* Used only with Voronoi diagram; out only */
		int numberofedges;                                             /* Out only */
	};

	void triangulate(char *, struct triangulateio *, struct triangulateio *, struct triangulateio *);
	void trifree(void* memptr);

	/** Destroys all dynamic generated buffer and set all values to zero. */
	void DestroyTriangulateCont(triangulateio& cont);


#endif//__SMARTPROJECTOR_SUPPORT_DELAUNAY_TRIANGLE_ESTIMATION__