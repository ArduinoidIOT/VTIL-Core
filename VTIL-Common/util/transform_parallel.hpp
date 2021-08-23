// Copyright (c) 2020 Can Boluk and contributors of the VTIL Project   
// All rights reserved.   
//    
// Redistribution and use in source and binary forms, with or without   
// modification, are permitted provided that the following conditions are met: 
//    
// 1. Redistributions of source code must retain the above copyright notice,   
//    this list of conditions and the following disclaimer.   
// 2. Redistributions in binary form must reproduce the above copyright   
//    notice, this list of conditions and the following disclaimer in the   
//    documentation and/or other materials provided with the distribution.   
// 3. Neither the name of VTIL Project nor the names of its contributors
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.   
//    
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE   
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR   
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF   
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN   
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)   
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  
// POSSIBILITY OF SUCH DAMAGE.        
//
#pragma once
#include <iterator>
#include <vector>
#include <future>
#include "type_helpers.hpp"
#include "intrinsics.hpp"

// [Configuration]
// Determine whether or not to use parallel transformations.
//
#ifndef VTIL_USE_PARALLEL_TRANSFORM
	#define VTIL_USE_PARALLEL_TRANSFORM false
#endif

namespace vtil
{
	namespace impl
	{
		template<typename T>
		__forceinline static decltype( auto ) ref_adjust( T&& x )
		{
			if constexpr ( std::is_reference_v<T> )
				return std::ref( x );
			else
				return x;
		}
	};

	// Generic parallel worker helper.
	//
	template<Iterable C, typename F> requires Invocable<F, void, iterator_reference_type_t<C>>
	static void transform_parallel( C&& container, const F& worker )
	{
		size_t container_size = std::size( container );

		// If parallel transformation is disabled or if the container only has one entry, 
		// fallback to serial transformation.
		//
		if ( !VTIL_USE_PARALLEL_TRANSFORM || container_size == 1 )
		{
			for ( auto it = std::begin( container ); it != std::end( container ); ++it )
				worker( *it );
		}
		// Otherwise, use std::future for each entry.
		//
		else
		{
			std::vector<std::future<void>> tasks;
			tasks.reserve( container_size );
			for ( auto it = std::begin( container ); it != std::end( container ); ++it )
				tasks.emplace_back( std::async( std::launch::async, std::ref( worker ), impl::ref_adjust( *it ) ) );

			// Call ::get before destruction to propagate exceptions
			//
			for ( auto& task : tasks )
				task.get();
		}
	}
};