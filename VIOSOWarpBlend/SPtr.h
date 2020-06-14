#ifndef VWB_SPTR_INCLUDE_HPP
#define VWB_SPTR_INCLUDE_HPP

/*! \brief A class to smart harness ordinary pointers
 *
 *  Use this class for automatic use of pointers.
 *  Pointers are automatically deleted if not used anymore.
 *  SPtr keeps an internal reference counter.
 */

#ifndef ASSERT
#define ASSERT(exp) (NULL)
#endif

template< class T >
class SPtr
{
public:
	volatile unsigned int* piRef;		//< the reference counter
	T* ptr;				//< the object pointer

	/**
	* Default Constructor
	* constructs an empty object
	*/
	SPtr()
	: ptr( NULL )
	, piRef( NULL ) 
	{
	}

	/**
	* Construct an object from another smartpointer of other template type, which must be a subclass
	* @param p pointer to a valid pointer of the template type, it always will be attached
	* do not delete original pointer afterwards! 
	*/
	template<class T2>
	SPtr( SPtr<T2> const& other )
	: ptr( dynamic_cast< T* >( other.ptr ) )
	, piRef( other.piRef )
	{ 
		if( NULL == ptr )
			piRef = NULL;
		else if( piRef )
			InterlockedIncrement( piRef );
	}

	/**
	* Copy constructor, actually a refinement of the above but necessary to overwrite implicit copy constructor
	* Construct an object from another object of the same type
	* @param other the object to make a copy from
	*/
	SPtr( SPtr const& other )
	: ptr( other.ptr )
	, piRef( other.piRef )
	{
		ASSERT( ( NULL == ptr && NULL == piRef ) || ( ptr && piRef ) );
		if( piRef )
			InterlockedIncrement( piRef ); 
	}

	/**
	* Construct an object from a value of the template type
	* @param v const reference to a value
	*/
	explicit SPtr( T const& v )
	: ptr( new T )
	, piRef( new volatile unsigned int ) 
	{ 
		*piRef = 1;
		*ptr = v; 
	}

	/**
	* Construct an object from a pointer of the template type
	* @param p pointer to a valid pointer of the template type, it always will be attached
	* do not delete original pointer afterwards! 
	*/
	SPtr( T* p )
	: ptr( p )
	, piRef( p ? new volatile unsigned int : NULL )
	{ 
		if( piRef )
			*piRef = 1; 
	}

	/**
	* Destructor
	*/
	~SPtr()
	{ 
		if( piRef && 
			0 == ::InterlockedDecrement( piRef ) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}
	}

	/**
	* operator bool
	* @return true if the containing pointer is not NULL
	*/
	operator bool() const { return NULL != ptr; }

	/**
	* operator ==
	* @param other smartpointer to compare me to
	* @return true if internal pointers are equal, thus pointing to the same literal
	*/
	bool operator ==( SPtr const& other ) const { return ptr == other.ptr; }

	/**
	* operator !=
	* @param other smartpointer to compare me to
	* @return true if internal pointers are not equal, thus not pointing to the same literal
	*/
	bool operator !=( SPtr const& other ) const { return ptr != other.ptr; }

	/**
	* operator =
	* Standard assignment operator
	* @param other const reference of another smartpointer
	* @return reference of this object
	*/
	SPtr& operator=( SPtr const& other ) 
	{ 
		if( piRef && 
			0 == InterlockedDecrement( piRef ) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}

		ptr = other.ptr; 
		piRef = other.piRef;
		if( piRef )
			InterlockedIncrement( piRef ); 
		return *this;
	}

	/**
	* operator =
	* assign some pointer of my type to be managed
	* the currently assiged pointer will be released
	* @param p some pointer of my type
	* @return reference of this object
	*/
	SPtr& operator=( T* p ) 
	{ 
		if( ptr == p  )
			return *this;

		if( piRef && 
			0 == InterlockedDecrement( piRef ) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}

		ptr = p; 
		if( p )
		{
			piRef = new volatile unsigned int; 
			*piRef = 1;
		}
		return *this;
	}

	/**
	* operator ->
	* operator to dereference the smartpointer like a pointer
	*/
	T* operator->() { return ptr; }

	/**
	* operator -> const
	* operator to dereference the smartpointer like a pointer
	*/
	T const* operator->() const { return ptr; }

	///**
	//* operator *
	//* operator to use the smartpointer like a pointer
	//*/
	//operator T*() { return ptr; }

	///**
	//* operator const *
	//* operator to use the smartpointer like a pointer
	//*/
	//operator const T*() const { return ptr; }

	/**
	* operator & const
	* operator to make & references to the containing pointer
	* @return a const pointer to the managed pointer
	*/
	T* const* operator&() const { return &ptr; }

	/**
	* operator &
	* operator to make & references to the containing pointer
	* this is used to assign a new value to the containing pointer.
	* This is used for all kinds of create routines.
	* It will ASSERT if the currently assigned pointer is not NULL, as it will probably
	* be overwritten and the former pointer gets lost.
	* Use &var.ptr instead, if you intend to use a not const reference.
	* @return a reference of the managed COM pointer
	*/
	T** operator&() 
	{ 
		ASSERT( NULL == piRef && NULL == ptr );
		piRef = new volatile unsigned int;
		*piRef = 1;
		return &ptr; 
	}

	/**
	* operator * const
	* dereferences the internal pointer
	*/
	T const& operator*() const { return *ptr; }

	/**
	* operator *
	* dereferences the internal pointer
	*/
	T& operator*() { return *ptr; }

	/**
	* Create
	* creates an empty smartpointer object
	* @return reference to this object
	*/
	SPtr& Create()
	{
		if( piRef && 0 == (InterlockedDecrement( piRef )) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}
		piRef = new volatile unsigned int;
		*piRef = 1;
		ptr = new T;
	}

	/**
	* Create
	* creates an smartpointer object
	* @param a value to the managed type, this will be used as parameter in the type's constructor
	* @return reference to this object
	*/
	SPtr& Create( T const& v )
	{
		if( piRef && 0 == (InterlockedDecrement( piRef )) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}
		piRef = new volatile unsigned int;
		*piRef = 1;
		ptr = new T( v );
	}

	/**
	* AddRef
	increase this smartpointer object, reference counter is decreased, the containing pointer is freed if no longer used
	*/
	SPtr& AddRef()
	{
		if( piRef )
			::InterlockedIncrement( piRef );
		return *this;
	}

	/**
	* Release
	empties this smartpointer object, reference counter is decreased, the containing pointer is freed if no longer used
	*/
	void Release()
	{
		if( piRef && 0 == (InterlockedDecrement( piRef )) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}
		ptr = NULL;
		piRef = NULL;
	}

	/** ref
	* get the current reference count of the managed pointer
	* @return current reference count of the managed pointer
	*/
	unsigned int ref() const { return piRef ? *piRef : 0; }

	/*
	* attach
	* attaches a pointer of my type to be managed
	* @param p pointer of my type, do not delete the given pointer afterwards
	* @return a reference to this object
	*/
	SPtr& attach( T* p )
	{ 
		if( piRef && 0 == (InterlockedDecrement( piRef )) )
		{
			delete piRef;
			if( ptr ) 
				delete ptr;
		}

		ptr = p; 
		piRef = new volatile unsigned int; 
		*piRef = 1;
		return *this; 
	}

	/*
	* detach
	* detaches the pointer from the object
	* The object will become empty after detaching.
	* This must be the only object managing this pointer, thus the reference counter has to be 1.
	* @return the pointer formally managed by the object
	*/
	T* detach() 
	{ 
		T* ret = ptr;
		if( piRef )
		{
			ASSERT( 1 == *piRef );
			delete piRef;
			piRef = NULL;
		}		
		ptr = NULL;
		return ret; 
	}
};


#endif //ndef VWB_SPTR_INCLUDE_HPP


