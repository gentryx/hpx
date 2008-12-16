//  Copyright (c) 2007-2008 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_SERVER_ACCUMULATOR_MAY_17_2008_0731PM)
#define HPX_COMPONENTS_SERVER_ACCUMULATOR_MAY_17_2008_0731PM

#include <iostream>

#include <hpx/hpx_fwd.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/threads/thread.hpp>
#include <hpx/runtime/components/component_type.hpp>
#include <hpx/runtime/components/server/managed_component_base.hpp>
#include <hpx/runtime/actions/component_action.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace server 
{
    ///////////////////////////////////////////////////////////////////////////
    /// The accumulator is a very simple example of a HPX component. 
    ///
    /// The accumulator class is a small example components demonstrating the
    /// main principles of writing your own components. It exposes 4 different
    /// actions: init, add, query, and print, showing how to used and implement
    /// functionality in a way conformant with the HPX runtime system. 
    ///
    /// Note that the implementation of the accumulator does not require any 
    /// special data members or virtual functions.
    ///
    class accumulator 
      : public components::detail::managed_component_base<accumulator> 
    {
    public:
        // parcel action code: the action to be performed on the destination 
        // object (the accumulator)
        enum actions
        {
            accumulator_init = 0,
            accumulator_add = 1,
            accumulator_query_value = 2,
            accumulator_print = 3
        };

        // constructor: initialize accumulator value
        accumulator()
          : arg_(0)
        {}

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Initialize the accumulator
        void init() 
        {
            arg_ = 0;
        }

        /// Add the given number to the accumulator
        void add (double arg) 
        {
            arg_ += arg;
        }

        /// Return the current value to the caller
        double query() 
        {
            return arg_;
        }

        /// Print the current value of the accumulator
        void print() 
        {
            std::cout << arg_ << std::flush << std::endl;
        }

        ///////////////////////////////////////////////////////////////////////
        // Each of the exposed functions needs to be encapsulated into an action
        // type, allowing to generate all required boilerplate code for threads,
        // serialization, etc.
        typedef hpx::actions::action0<
            accumulator, accumulator_init, &accumulator::init
        > init_action;

        typedef hpx::actions::action1<
            accumulator, accumulator_add, double, &accumulator::add
        > add_action;

        typedef hpx::actions::result_action0<
            accumulator, double, accumulator_query_value, &accumulator::query
        > query_action;

        typedef hpx::actions::action0<
            accumulator, accumulator_print, &accumulator::print
        > print_action;

    private:
        double arg_;
    };

}}}

#endif
