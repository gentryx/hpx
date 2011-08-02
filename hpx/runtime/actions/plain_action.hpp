//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_RUNTIME_ACTIONS_PLAIN_ACTION_NOV_14_2008_0706PM)
#define HPX_RUNTIME_ACTIONS_PLAIN_ACTION_NOV_14_2008_0706PM

#include <cstdlib>
#include <stdexcept>

#include <hpx/hpx_fwd.hpp>
#include <hpx/config.hpp>
#include <hpx/exception.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/actions/continuation.hpp>
#include <hpx/runtime/actions/action_support.hpp>
#include <hpx/runtime/components/server/plain_function.hpp>
#include <hpx/util/unused.hpp>

#include <boost/version.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repeat.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace actions
{
    ///////////////////////////////////////////////////////////////////////////
#define HPX_FUNCTION_ARG_ENUM(z, n, data)                                     \
        BOOST_PP_CAT(function_action_arg, BOOST_PP_INC(n)) =                  \
            function_action_base + BOOST_PP_INC(n),                           \
    /**/
#define HPX_FUNCTION_RETARG_ENUM(z, n, data)                                  \
        BOOST_PP_CAT(function_result_action_arg, BOOST_PP_INC(n)) =           \
            function_result_action_base + BOOST_PP_INC(n),                    \
    /**/

    enum function_action
    {
        /// plain (free) remotely callable function identifiers
        function_action_base = 100,
        function_action_arg0 = function_action_base + 0, 
        BOOST_PP_REPEAT(HPX_ACTION_ARGUMENT_LIMIT, HPX_FUNCTION_ARG_ENUM, _)

        /// plain (free) remotely callable function identifiers with result
        function_result_action_base = 200,
        function_result_action_arg0 = function_result_action_base + 0, 
        BOOST_PP_REPEAT(HPX_ACTION_ARGUMENT_LIMIT, HPX_FUNCTION_RETARG_ENUM, _)
    };

#undef HPX_FUNCTION_RETARG_ENUM
#undef HPX_FUNCTION_ARG_ENUM

    ///////////////////////////////////////////////////////////////////////////
    //  Specialized generic plain (free) action types allowing to hold a 
    //  different number of arguments
    ///////////////////////////////////////////////////////////////////////////

    // zero argument version
    template <typename Result, Result (*F)(), typename Derived, 
      threads::thread_priority Priority = threads::thread_priority_default>
    class plain_base_result_action0 
      : public action<
            components::server::plain_function<Derived>, 
            function_result_action_arg0, Result, boost::fusion::vector<>,
            Derived, Priority>
    {
    public:
        typedef Result result_type;
        typedef boost::fusion::vector<> arguments_type;
        typedef action<
            components::server::plain_function<Derived>, 
            function_result_action_arg0, result_type, arguments_type,
            Derived, Priority
        > base_type;

        explicit plain_base_result_action0(threads::thread_priority priority = Priority)
          : base_type(priority)
        {}

    protected:
        /// The \a thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the 
        /// original function (given by \a func), while ignoring the return 
        /// value.
        template <typename State>   // dummy template parameter
        static threads::thread_state_enum 
        thread_function(State)
        {
            try {
                LTM_(debug) << "Executing plain action("
                            << detail::get_action_name<Derived>()
                            << ").";
                F();      // call the function, ignoring the return value
            }
            catch (hpx::exception const& e) {
                LTM_(error) 
                    << "Unhandled exception while executing plain action("
                    << detail::get_action_name<Derived>()
                    << "): " << e.what();
            }
            return threads::terminated;
        }

    public:
        typedef boost::mpl::false_ direct_execution;

        /// \brief This static \a construct_thread_function allows to construct 
        /// a proper thread function for a \a thread without having to 
        /// instantiate the \a plain_base_result_action0 type. This is used by 
        /// the \a applier in case no continuation has been supplied.
        static boost::function<threads::thread_function_type> 
        construct_thread_function(naming::address::address_type lva)
        {
            // we need to assign the address of the thread function to a 
            // variable to  help the compiler to deduce the function type
            threads::thread_state_enum (*f)(threads::thread_state_ex_enum) =
                &Derived::template thread_function<threads::thread_state_ex_enum>;
            return f;
        }

        /// \brief This static \a construct_thread_function allows to construct 
        /// a proper thread function for a \a thread without having to 
        /// instantiate the \a base_result_action0 type. This is used by the \a 
        /// applier in case a continuation has been supplied
        static boost::function<threads::thread_function_type> 
        construct_thread_function(continuation_type& cont, 
            naming::address::address_type lva)
        {
            return base_type::construct_continuation_thread_function(F, cont);
        }

        /// serialization support
        static void register_base()
        {
            using namespace boost::serialization;
            void_cast_register<plain_base_result_action0, base_type>();
            base_type::register_base();
        }

    private:
        // serialization support    
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }

    private:
        /// This \a get_thread_function will be invoked to retrieve the thread 
        /// function for an action which has to be invoked without continuations.
        boost::function<threads::thread_function_type>
        get_thread_function(naming::address::address_type lva) const
        {
            return construct_thread_function(lva);
        }

        /// This \a get_thread_function will be invoked to retrieve the thread 
        /// function for an action which has to be invoked with continuations.
        boost::function<threads::thread_function_type>
        get_thread_function(continuation_type& cont,
            naming::address::address_type lva) const
        {
            return construct_thread_function(cont, lva);
        }

        /// This \a get_thread_function will be invoked to retrieve the thread 
        /// function for an action which has to be invoked without continuations.
        boost::function<threads::thread_function_type>
        get_thread_function(naming::address::address_type lva,
            arguments_type const& arg) const
        {
            return construct_thread_function(lva);
        }

        /// This \a get_thread_function will be invoked to retrieve the thread 
        /// function for an action which has to be invoked with continuations.
        boost::function<threads::thread_function_type>
        get_thread_function(continuation_type& cont,
            naming::address::address_type lva, arguments_type const& arg) const
        {
            return construct_thread_function(cont, lva);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Result, Result (*F)(),
        threads::thread_priority Priority = threads::thread_priority_default,
        typename Derived = detail::this_type>
    class plain_result_action0 
      : public plain_base_result_action0<Result, F, 
            typename detail::action_type<
                plain_result_action0<Result, F, Priority>, Derived
            >::type, Priority>
    {
    private:
        typedef typename detail::action_type<
            plain_result_action0<Result, F, Priority>, Derived
        >::type derived_type;

        typedef plain_base_result_action0<Result, F, derived_type, Priority> 
            base_type;

    public:
        explicit plain_result_action0(threads::thread_priority priority = Priority)
          : base_type(priority)
        {}

        Result execute_function(naming::address::address_type lva) const {
            LTM_(debug)
                << "plain_result_action0::execute_function: name(" 
                << detail::get_action_name<derived_type>()
                << ")";
            return F();
        }

        static Result execute_function_nonvirt(naming::address::address_type lva)
        {
            LTM_(debug)
                << "plain_result_action0::execute_function_nonvirt: name(" 
                << detail::get_action_name<derived_type>()
                << ")";
            return F();
        }

        /// serialization support
        static void register_base()
        {
            using namespace boost::serialization;
            void_cast_register<plain_result_action0, base_type>();
            base_type::register_base();
        }

    private:
        // serialization support
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }

    private:
        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(cont, lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(lva, data);
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(cont, lva, data);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Result, Result (*F)(),
        typename Derived = detail::this_type>
    class plain_direct_result_action0 
      : public plain_base_result_action0<Result, F, 
            typename detail::action_type<
                plain_direct_result_action0<Result, F>, Derived
            >::type>
    {
    private:
        typedef typename detail::action_type<
            plain_direct_result_action0<Result, F>, Derived
        >::type derived_type;

        typedef plain_base_result_action0<Result, F, derived_type> base_type;

    public:
        plain_direct_result_action0()
        {}

        explicit plain_direct_result_action0(threads::thread_priority)
        {}

    public:
        typedef boost::mpl::true_ direct_execution;

        Result execute_function(naming::address::address_type lva) const
        {
            LTM_(debug)
                << "plain_direct_result_action0::execute_function: name(" 
                << detail::get_action_name<derived_type>()
                << ")";
            return F();
        }

        static Result execute_function_nonvirt(naming::address::address_type lva)
        {
            LTM_(debug)
                << "plain_direct_result_action0::execute_function_nonvirt: name(" 
                << detail::get_action_name<derived_type>()
                << ")";
            return F();
        }

        /// serialization support
        static void register_base()
        {
            using namespace boost::serialization;
            void_cast_register<plain_direct_result_action0, base_type>();
            base_type::register_base();
        }

    private:
        // serialization support
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }

    private:
        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        base_action::action_type get_action_type() const 
        {
            return base_action::direct_action;
        }

        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(cont, lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(lva, data);
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(cont, lva, data);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  zero parameter version, no result value
    template <void (*F)(), typename Derived, 
      threads::thread_priority Priority = threads::thread_priority_default>
    class plain_base_action0 
      : public action<
            components::server::plain_function<Derived>, 
            function_action_arg0, util::unused_type, boost::fusion::vector<>,
            Derived, Priority>
    {
    public:
        typedef util::unused_type result_type;
        typedef boost::fusion::vector<> arguments_type;
        typedef action<
            components::server::plain_function<Derived>, 
            function_action_arg0, result_type, arguments_type,
            Derived, Priority> base_type;

        explicit plain_base_action0(threads::thread_priority priority = Priority)
          : base_type(priority)
        {}

    protected:
        /// The \a continuation_thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the 
        /// original function (given by \a func), while ignoring the return 
        /// value.
        template <typename State>   // dummy template parameter
        static threads::thread_state_enum 
        thread_function(State)
        {
            try {
                LTM_(debug) << "Executing plain action("
                            << detail::get_action_name<Derived>()
                            << ").";
                F();      // call the function, ignoring the return value
            }
            catch (hpx::exception const& e) {
                LTM_(error) 
                    << "Unhandled exception while executing plain action("
                    << detail::get_action_name<Derived>()
                    << "): " << e.what();
            }
            return threads::terminated;
        }

    public:
        typedef boost::mpl::false_ direct_execution;

        /// \brief This static \a construct_thread_function allows to construct 
        /// a proper thread function for a \a thread without having to 
        /// instantiate the base_action0 type. This is used by the \a applier in 
        /// case no continuation has been supplied.
        static boost::function<threads::thread_function_type> 
        construct_thread_function(naming::address::address_type lva)
        {
            // we need to assign the address of the thread function to a 
            // variable to  help the compiler to deduce the function type
            threads::thread_state_enum (*f)(threads::thread_state_ex_enum) =
                &Derived::template thread_function<threads::thread_state_ex_enum>;
            return f;
        }

        /// \brief This static \a construct_thread_function allows to construct 
        /// a proper thread function for a \a thread without having to 
        /// instantiate the base_action0 type. This is used by the \a applier in 
        /// case a continuation has been supplied
        static boost::function<threads::thread_function_type> 
        construct_thread_function(continuation_type& cont,
            naming::address::address_type lva)
        {
            return base_type::construct_continuation_thread_function_void(F, cont);
        }

        /// serialization support
        static void register_base()
        {
            using namespace boost::serialization;
            void_cast_register<plain_base_action0, base_type>();
            base_type::register_base();
        }

    private:
        boost::function<threads::thread_function_type>
        get_thread_function(naming::address::address_type lva) const
        {
            return construct_thread_function(lva);
        }

        boost::function<threads::thread_function_type>
        get_thread_function(continuation_type& cont,
            naming::address::address_type lva) const
        {
            return construct_thread_function(cont, lva);
        }

        boost::function<threads::thread_function_type>
        get_thread_function(naming::address::address_type lva,
            arguments_type const& arg) const
        {
            return construct_thread_function(lva);
        }

        boost::function<threads::thread_function_type>
        get_thread_function(continuation_type& cont,
            naming::address::address_type lva, arguments_type const& arg) const
        {
            return construct_thread_function(cont, lva);
        }

    private:
        // serialization support
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <void (*F)(),
        threads::thread_priority Priority = threads::thread_priority_default,
        typename Derived = detail::this_type>
    class plain_action0 
      : public plain_base_action0<F, 
            typename detail::action_type<
                plain_action0<F, Priority>, Derived
            >::type, Priority>
    {
    private:
        typedef typename detail::action_type<
            plain_action0<F, Priority>, Derived
        >::type derived_type;

        typedef plain_base_action0<F, derived_type, Priority> base_type;

    public:
        explicit plain_action0(threads::thread_priority priority = Priority)
          : base_type(priority)
        {}

        util::unused_type
        execute_function(naming::address::address_type lva) const
        {
            LTM_(debug)
                << "plain_action0::execute_function: name("
                << detail::get_action_name<derived_type>()
                << ")";
            F();
            return util::unused;
        }

        static util::unused_type
        execute_function_nonvirt(naming::address::address_type lva)
        {
            LTM_(debug)
                << "plain_action0::execute_function_nonvirt: name("
                << detail::get_action_name<derived_type>()
                << ")";
            F();
            return util::unused;
        }

        /// serialization support
        static void register_base()
        {
            using namespace boost::serialization;
            void_cast_register<plain_action0, base_type>();
            base_type::register_base();
        }

    private:
        // serialization support
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }

    private:
        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(cont, lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(lva, data);
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(cont, lva, data);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <void (*F)(), typename Derived = detail::this_type>
    class plain_direct_action0 
      : public plain_base_action0<F, 
            typename detail::action_type<
                plain_direct_action0<F>, Derived
            >::type>
    {
    private:
        typedef typename detail::action_type<
            plain_direct_action0<F>, Derived
        >::type derived_type;

        typedef plain_base_action0<F, derived_type> base_type;

    public:
        plain_direct_action0()
        {}

        explicit plain_direct_action0(threads::thread_priority)
        {}

    public:
        typedef boost::mpl::true_ direct_execution;

        util::unused_type
        execute_function(naming::address::address_type lva) const
        {
            LTM_(debug)
                << "plain_base_action0::execute_function: name("
                << detail::get_action_name<derived_type>()
                << ")";
            F();
            return util::unused;
        }

        static util::unused_type
        execute_function_nonvirt(naming::address::address_type lva)
        {
            LTM_(debug)
                << "plain_base_action0::execute_function_nonvirt: name("
                << detail::get_action_name<derived_type>()
                << ")";
            F();
            return util::unused;
        }

        /// serialization support
        static void register_base()
        {
            using namespace boost::serialization;
            void_cast_register<plain_direct_action0, base_type>();
            base_type::register_base();
        }

    private:
        // serialization support
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int /*version*/)
        {
            ar & boost::serialization::base_object<base_type>(*this);
        }

    private:
        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        base_action::action_type get_action_type() const 
        {
            return base_action::direct_action;
        }

        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data)
        {
            data.lva = lva;
            data.func = this->construct_thread_function(cont, lva);
            data.description = detail::get_action_name<derived_type>();
            data.parent_id = reinterpret_cast<threads::thread_id_type>(this->parent_id_);
            data.parent_phase = this->parent_phase_;
            data.parent_prefix = this->parent_locality_;
            data.priority = this->priority_; 
            return data;
        }

        threads::thread_init_data& 
        get_thread_init_data(naming::address::address_type lva,
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(lva, data);
        }

        threads::thread_init_data& 
        get_thread_init_data(continuation_type& cont,
            naming::address::address_type lva, 
            threads::thread_init_data& data,
            typename base_type::arguments_type const&)
        {
            return this->get_thread_init_data(cont, lva, data);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // bring in the rest of the implementations
    #include <hpx/runtime/actions/plain_action_implementations.hpp>

///////////////////////////////////////////////////////////////////////////////
}}

#include <hpx/config/warnings_suffix.hpp>

#endif

