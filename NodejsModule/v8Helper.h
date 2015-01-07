#ifndef __V8_HELPER
#define __V8_HELPER

#include "v8.h"
#include "v8-convert.hpp"
#include <node.h>

#include <string>
#include <iostream>
#include <arpa/inet.h>

using namespace std;

// For invocation of standalone (non-member) functions,
// zero-argument version.
// NOTE: if partial specialization will be required for non-member functions, duplicate the
// "funny template approach" below that is set up for emulation of partial specialization
// of member functions.  Everything would be the same except without the 
// "typename T" and the "T::"
template <typename NATIVE_RETURN,  NATIVE_RETURN (*F)()> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments __attribute((__unused__)) & args)
{
	using namespace v8;
	HandleScope scope;

	Handle<v8::Value> result = cvv8::CastToJS(F());
	return scope.Close(result);
}   

// Invocation of standalone (non-member) functions,
// one-argument version version.
// See note above regarding partial specialization, if it is ever required.
template <typename NATIVE_RETURN, typename NATIVE_P1, NATIVE_RETURN (*F)(NATIVE_P1)> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
	using namespace v8;
	HandleScope scope;

	if( args.Length() < 1 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
	}

	NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>( args[0] );

	Handle<v8::Value> result = cvv8::CastToJS(F(p1));
	return scope.Close(result);
}


// Invocation of standalone (non-member) functions,
// two-argument version version.
template <typename NATIVE_RETURN, typename NATIVE_P1, typename NATIVE_P2, NATIVE_RETURN (*F)(NATIVE_P1, NATIVE_P2)> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
	using namespace v8;
	HandleScope scope;

	if( args.Length() < 2 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
	}

	NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>( args[0] );
	NATIVE_P2 p2 = cvv8::CastFromJS<NATIVE_P2>( args[1] );

	Handle<v8::Value> result = cvv8::CastToJS(F(p1, p2));
	return scope.Close(result);
}

// three-argument version version.
template <typename NATIVE_RETURN, typename NATIVE_P1, typename NATIVE_P2, typename NATIVE_P3, NATIVE_RETURN (*F)(NATIVE_P1, NATIVE_P2, NATIVE_P3)> 
v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
	using namespace v8;
	HandleScope scope;

	if( args.Length() < 3 )
	{
		return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameters")));
	}

	NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>( args[0] );
	NATIVE_P2 p2 = cvv8::CastFromJS<NATIVE_P2>( args[1] );
	NATIVE_P3 p3 = cvv8::CastFromJS<NATIVE_P2>( args[2] );

	Handle<v8::Value> result = cvv8::CastToJS(F(p1, p2, p3));
	return scope.Close(result);
}






// Invocation of member methods,
// zero-argument version.
//
// Funny template approach is used to enable emulated partial specialization.
template <typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
struct InvokeMethod_impl
{
	static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;
		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());

		Handle<v8::Value> result = cvv8::CastToJS(  (  (nativeHandler)->*(F)  )()  );
		return scope.Close(result);
		}
};

// Invocation of member methods,
// one-argument version.
template <typename NATIVE_RETURN, typename T, typename NATIVE_P1, NATIVE_RETURN (T::*F)(NATIVE_P1)> 
struct InvokeMethod_impl_1
{
	static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;
		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());

		if( args.Length() < 1 )
		{
			return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
		}

		NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>(args[0]);

		Handle<v8::Value> result = cvv8::CastToJS(  (  (nativeHandler)->*(F)  )(p1)  );

		return scope.Close(result);
		}
};

// Invocation of member methods,
// zero argument version.
// Specialization for in_addr return type.
template<typename T, in_addr (T::*F)(void)>
struct InvokeMethod_impl<in_addr, T, F>
{
	static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;
		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());

		in_addr addr = (nativeHandler->*(F))();
		char* addrAsString = inet_ntoa(addr);
		Local<String> result = Local<String>::New( String::New( addrAsString ));
		return scope.Close(result);
		}
};

// Invocation of member methods,
// zero argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, NATIVE_RETURN (T::*F)(void)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
	return InvokeMethod_impl<NATIVE_RETURN, T, F >::InvokeMethod(args);
}

// Invocation of member methods,
// one argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, typename NATIVE_P1, NATIVE_RETURN (T::*F)(NATIVE_P1)> 
static v8::Handle<v8::Value> InvokeMethod(const v8::Arguments& args)
{
	return InvokeMethod_impl_1<NATIVE_RETURN, T, NATIVE_P1, F >::InvokeMethod(args);
}











// Invocation of member methods,
// zero-argument version.
//
// Funny template approach is used to enable emulated partial specialization.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, NATIVE_RETURN (CHILD_TYPE::*F)(void)> 
struct InvokeWrappedMethod_impl
{
	static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;

		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());
		CHILD_TYPE *handler = nativeHandler->GetChild();

		Handle<v8::Value> result = cvv8::CastToJS(  (  (handler)->*(F)  )()  );
		return scope.Close(result);
		}
};


// Invocation of member methods,
// one-argument version.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1)> 
struct InvokeWrappedMethod_impl_1
{
	static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;
		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());


		if( args.Length() < 1 )
		{
			return ThrowException(Exception::TypeError(String::New("Must be invoked with one parameter")));
		}

		NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>(args[0]);


		CHILD_TYPE *handler = nativeHandler->GetChild();

		Handle<v8::Value> result = cvv8::CastToJS(  (  (handler)->*(F)  )(p1)  );
		return scope.Close(result);
		}
};


// Invocation of member methods,
// two-argument version.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, typename NATIVE_P2, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1, NATIVE_P2)> 
struct InvokeWrappedMethod_impl_2
{
	static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;
		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());

		if( args.Length() < 2 )
		{
			return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameter")));
		}

		NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>(args[0]);
		NATIVE_P2 p2 = cvv8::CastFromJS<NATIVE_P2>(args[1]);


		CHILD_TYPE *handler = nativeHandler->GetChild();

		Handle<v8::Value> result = cvv8::CastToJS(  (  (handler)->*(F)  )(p1, p2)  );
		return scope.Close(result);
		}
};

// Invocation of member methods,
// three-argument version.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, typename NATIVE_P2, typename NATIVE_P3, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1, NATIVE_P2, NATIVE_P3)>
struct InvokeWrappedMethod_impl_3
{
	static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
		{
		using namespace v8;
		HandleScope scope;
		T* nativeHandler = node::ObjectWrap::Unwrap<T>(args.This());

		if( args.Length() < 2 )
		{
			return ThrowException(Exception::TypeError(String::New("Must be invoked with two parameter")));
		}

		NATIVE_P1 p1 = cvv8::CastFromJS<NATIVE_P1>(args[0]);
		NATIVE_P2 p2 = cvv8::CastFromJS<NATIVE_P2>(args[1]);
		NATIVE_P2 p3 = cvv8::CastFromJS<NATIVE_P2>(args[2]);


		CHILD_TYPE *handler = nativeHandler->GetChild();

		Handle<v8::Value> result = cvv8::CastToJS(  (  (handler)->*(F)  )(p1, p2, p3)  );
		return scope.Close(result);
		}
};

// Invocation of member methods,
// zero argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, NATIVE_RETURN (CHILD_TYPE::*F)(void)> 
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
	return InvokeWrappedMethod_impl<NATIVE_RETURN, T, CHILD_TYPE, F >::InvokeWrappedMethod(args);
}

// Invocation of member methods,
// one argument version.
//
// This wrapper template function along with the classes above
// emulates partial specialization where required.
template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1)> 
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
	return InvokeWrappedMethod_impl_1<NATIVE_RETURN, T, CHILD_TYPE, NATIVE_P1, F >::InvokeWrappedMethod(args);
}

template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, typename NATIVE_P2, NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1, NATIVE_P2)> 
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
	return InvokeWrappedMethod_impl_2<NATIVE_RETURN, T, CHILD_TYPE, NATIVE_P1, NATIVE_P2, F >::InvokeWrappedMethod(args);
}

template <typename NATIVE_RETURN, typename T, typename CHILD_TYPE, typename NATIVE_P1, typename NATIVE_P2, typename NATIVE_P3,NATIVE_RETURN (CHILD_TYPE::*F)(NATIVE_P1, NATIVE_P2, NATIVE_P3)>
static v8::Handle<v8::Value> InvokeWrappedMethod(const v8::Arguments& args)
{
	return InvokeWrappedMethod_impl_3<NATIVE_RETURN, T, CHILD_TYPE, NATIVE_P1, NATIVE_P2, NATIVE_P3, F >::InvokeWrappedMethod(args);
}



#endif // __V8_HELPER

