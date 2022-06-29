#pragma save_types, strong_types, rtt_checks

#include "/sys/configuration.h"
#include "/sys/driver_info.h"
#include "/sys/object_info.h"

#include "/inc/base.inc"
#include "/inc/deep_eq.inc"
#include "/inc/gc.inc"
#include "/inc/testarray.inc"

lwobject var;

void run_test()
{
    int errors;
    int finished;

    msg("\nRunning test for lightweight objects:\n"
          "-------------------------------------\n");

    run_array(({
        ({ "driver_info(DI_NUM_LWOBJECTS) without lwobjects", 0,
            function int()
            {
                return driver_info(DI_NUM_LWOBJECTS) == 0;
            }
        }),
        ({ "driver_info(DI_SIZE_LWOBJECTS) without lwobjects", 0,
            function int()
            {
                return driver_info(DI_SIZE_LWOBJECTS) == 0;
            }
        }),
        ({ "creating a lightweight object", 0,
            function int()
            {
                lwobject "/lwo/stack" lwob = new_lwobject("/lwo/stack");
                return lwob != 0;
            },
        }),
        ({ "creating a lightweight object with error during __INIT()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                master.activate_error_on_init();
                new_lwobject("/lwo/error");
            }
        }),
        ({ "creating a lightweight object with error during new()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                master.activate_error_on_new();
                new_lwobject("/lwo/error");
            }
        }),
        ({ "checking runtime type", TF_ERROR,
            function void()
            {
                lwobject "/lwo/test" lwob = new_lwobject("/lwo/stack");
            },
        }),
        ({ "closure as H_CREATE_LWOBJECT 1", 0,
            function int()
            {
                lwobject lwob;

                set_driver_hook(H_CREATE_LWOBJECT, unbound_lambda(({'lwob, 'val}),
                    ({#',,
                        ({ #'call_strict, 'lwob, "push", 'val }),
                        ({ #'([, ({"Leak test"}) })
                    })));
                lwob = new_lwobject("/lwo/stack", "X");

                set_driver_hook(H_CREATE_LWOBJECT, "new"); /* Reset it. */
                return lwob.pop() == "X";
            }
        }),
        ({ "closure as H_CREATE_LWOBJECT 2", 0,
            function int()
            {
                lwobject lwob;

                set_driver_hook(H_CREATE_LWOBJECT, unbound_lambda(0,
                    ({#',,
                        ({ #'call_strict, ({#'this_object}), "push", "Y" }),
                        ({ #'([, ({"Leak test"}) })
                    })));
                lwob = new_lwobject("/lwo/stack");

                set_driver_hook(H_CREATE_LWOBJECT, "new"); /* Reset it. */
                return lwob.pop() == "Y";
            }
        }),
        ({ "closure as H_CREATE_LWOBJECT 3", 0,
            function int()
            {
                lwobject lwob;

                set_driver_hook(H_CREATE_LWOBJECT, function void(lwobject lwob, varargs mixed* values)
                {
                    foreach(mixed val: values)
                        lwob.push(val);
                });
                lwob = new_lwobject("/lwo/stack", 11, 22);

                set_driver_hook(H_CREATE_LWOBJECT, "new"); /* Reset it. */
                return lwob.pop() == 22 && lwob.pop() == 11 && lwob.empty();
            }
        }),
        ({ "checking variable initialization with share_variables", 0,
            function int()
            {
                return new_lwobject("/lwo/share_var").check_lwobject();
            }
        }),
        ({ "call_out()", 0,
            function int() : int finished = &finished
            {
                new_lwobject("/lwo/callout", function void() : int finished = &finished
                {
                    if (finished)
                        shutdown(0);
                });

                return 1;
            }
        }),
        ({ "call_out_info() / find_call_out()", 0,
            function int()
            {
                /* Check the call_out from /lwo/callout. */
                mixed* lwo_callouts = filter(call_out_info(),
                                        function int(mixed* co) { return lwobjectp(co[0]); });

                return sizeof(lwo_callouts) == 1
                    && closurep(lwo_callouts[0][1])
                    && find_call_out(lwo_callouts[0][1]) >= 0;
            }
        }),
        ({ "call_out() with non-existent function", 0,
           function int()
           {
              last_rt_warning = 0;
              new_lwobject("/lwo/tests").start_co();
              return sizeof(last_rt_warning);
           }
        }),
        ({ "UID of lightweight objects", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                return getuid(lwob) == "lwuid" && geteuid(lwob) == "lwuid";
            }
        }),
        ({ "calling a lightweight object with .", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                lwob.push("Here");
                lwob.push("I");
                lwob.push("Am");
                lwob.push("!");

                return lwob.pop() == "!";
            }
        }),
        ({ "calling a lightweight object several times with .", 0,
            function int()
            {
                /* This is for testing the call cache. */
                lwobject "/lwo/stack" lwob = new_lwobject("/lwo/stack");
                foreach(int i: 3)
                    lwob.push(i);
                return lwob.pop() == 2 && lwob.pop() == 1 && lwob.pop() == 0;
            }
        }),
        ({ "calling a missing function in a lightweight object several times with .", 0,
            function int()
            {
                lwobject "/lwo/stack" lwob = new_lwobject("/lwo/stack");
                foreach(int i: 3)
                    if (!catch(lwob.append(i)))
                        return 0;
                return 1;
            }
        }),
        ({ "calling a lightweight object with ->", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                lwob->push("Here");
                lwob->push("I");
                lwob->push("Am");
                lwob->push("!");

                return lwob->pop() == "!";
            }
        }),
        ({ "calling a lightweight object several times with ->", 0,
            function int()
            {
                /* This is for testing the call cache. */
                lwobject "/lwo/stack" lwob = new_lwobject("/lwo/stack");
                foreach(int i: 3)
                    lwob->push(i);
                return lwob->pop() == 2 && lwob->pop() == 1 && lwob->pop() == 0;
            }
        }),
        ({ "calling a missing function in a lightweight object several times with ->", 0,
            function int()
            {
                lwobject "/lwo/stack" lwob = new_lwobject("/lwo/stack");
                foreach(int i: 3)
                    if (lwob->append(i))
                        return 0;
                return 1;
            }
        }),
        ({ "misleading cached calls", 0,
            function int()
            {
#pragma no_rtt_checks
                lwobject "/lwo/stack" lwob;
                mixed ob = load_object("/lwo/stack"); /* Object. */
                lwob = ob;

                lwob.push("Haha");

                return ob.pop() == "Haha";
#pragma rtt_checks
            }
        }),
        ({ "call_other()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                call_other(lwob, "push", "A");
                return call_other(lwob, "top") == "A";
            }
        }),
        ({ "call_direct()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                call_direct(lwob, "push", "A");
                return call_direct(lwob, "top") == "A";
            }
        }),
        ({ "call_strict()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                call_strict(lwob, "push", "A");
                return call_strict(lwob, "top") == "A";
            }
        }),
        ({ "call_direct_strict()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                call_direct_strict(lwob, "push", "A");
                return call_direct_strict(lwob, "top") == "A";
            }
        }),
        ({ "call_resolved()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                mixed result;

                if (!call_resolved(&result, lwob, "push", "A"))
                    return 0;
                if (!call_resolved(&result, lwob, "top"))
                    return 0;
                return result == "A";
            }
        }),
        ({ "call_direct_resolved()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                mixed result;

                if (!call_direct_resolved(&result, lwob, "push", "A"))
                    return 0;
                if (!call_direct_resolved(&result, lwob, "top"))
                    return 0;
                return result == "A";
            }
        }),
        ({ "call_other() with arrays", 0,
            function int()
            {
                lwobject* lwobs = ({ new_lwobject("/lwo/stack"), new_lwobject("/lwo/stack") });
                lwobs->push("X");
                return deep_eq(lwobs->pop(), ({"X", "X"}));
            }
        }),
        ({ "filter()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/tests");
                int num = lwob.get();
                return deep_eq(filter(({num-1,num,num+1}), "check", lwob), ({num}));
            }
        }),
        ({ "copy()", 0,
            function int()
            {
                lwobject orig = new_lwobject("/lwo/stack");
                lwobject clone;

                orig.push("What?");
                clone = copy(orig);
                return clone.pop() == "What?" && clone.empty() && !orig.empty();
            }
        }),
        ({ "copy() with error during __INIT()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                lwobject orig = new_lwobject("/lwo/error");

                master.activate_error_on_init();
                copy(orig);
            }
        }),
        ({ "copy() with error during copied()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                lwobject orig = new_lwobject("/lwo/error");

                master.activate_error_on_copy();
                copy(orig);
            }
        }),
        ({ "deep_copy()", 0,
            function int()
            {
                lwobject orig = new_lwobject("/lwo/stack");
                lwobject clone;
                mapping value = (["What?"]), result;

                orig.push(value);
                clone = deep_copy(orig);
                result = clone.pop();

                return deep_eq(result, value) && result != value && clone.empty() && !orig.empty();
            }
        }),
        ({ "deep_copy() with error during __INIT()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                lwobject orig = new_lwobject("/lwo/error");

                master.activate_error_on_init();
                deep_copy(({orig}));
            }
        }),
        ({ "deep_copy() with error during copied()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                lwobject orig = new_lwobject("/lwo/error");

                master.activate_error_on_copy();
                deep_copy(({orig}));
            }
        }),
        ({ "saving and restoring", 0,
            function int()
            {
                lwobject orig = new_lwobject("/lwo/stack");
                lwobject clone;

                orig.push("What?");
                clone = restore_value(save_value(orig));
                return clone.pop() == "What?" && clone.empty() && !orig.empty();
            }
        }),
        ({ "saving and restoring in a mapping", 0,
            function int()
            {
                lwobject orig = new_lwobject("/lwo/stack");
                lwobject clone;

                orig.push("What?");
                clone = restore_value(save_value(([1:orig])))[1];
                return clone.pop() == "What?" && clone.empty() && !orig.empty();
            }
        }),
        ({ "saving and restoring in an array", 0,
            function int()
            {
                lwobject orig = new_lwobject("/lwo/stack");
                lwobject clone;

                orig.push("What?");
                clone = restore_value(save_value(({orig})))[0];
                return clone.pop() == "What?" && clone.empty() && !orig.empty();
            }
        }),
        ({ "saving and restoring in an lwobject", 0,
            function int()
            {
                lwobject outer = new_lwobject("/lwo/stack");
                lwobject inner = new_lwobject("/lwo/stack");
                lwobject clone;

                outer.push(inner);
                inner.push("What?");
                clone = restore_value(save_value(outer));
                return clone.pop().pop() == "What?" && clone.empty() && !outer.empty() && !inner.empty();
            }
        }),
        ({ "saving and restoring with double variables", 0,
            function int()
            {
                lwobject clone;

                // Throwing errors is okay.
                if (catch(clone = restore_value("(*\"/lwo/stack.c stack,stack\",({\"Stack1\",}),({\"Stack2\",}),*)")))
                    return 1;

                // Otherwise it must be a valid result.
                return clone.pop() == "Stack2" && clone.empty();
            }
        }),
        ({ "saving and restoring with error during __INIT()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                lwobject outer = new_lwobject("/lwo/stack");
                lwobject inner = new_lwobject("/lwo/error");
                outer.push(inner);

                master.activate_error_on_init();
                restore_value(save_value(outer));
            }
        }),
        ({ "saving and restoring with error during restored()", TF_ERROR,
            function void()
            {
                // Foremost we check that nothing leaks.
                object master = load_object("/lwo/error");
                lwobject outer = new_lwobject("/lwo/stack");
                lwobject inner = new_lwobject("/lwo/error");
                outer.push(inner);

                master.activate_error_on_restore();
                restore_value(save_value(outer));
            }
        }),
        ({ "blueprint()", 0,
            function int()
            {
                return blueprint(new_lwobject("/lwo/stack")) == find_object("/lwo/stack");
            }
        }),
        ({ "program_name()", 0,
            function int()
            {
                return program_name(new_lwobject("/lwo/stack")) == "/lwo/stack.c";
            }
        }),
        ({ "load_name()", 0,
            function int()
            {
                return load_name(new_lwobject("/lwo/stack")) == "/lwo/stack";
            }
        }),
        ({ "sprintf()",0,
            function int()
            {
                return strstr(sprintf("%O", new_lwobject("/lwo/stack")), "stack");
            }
        }),
        ({ "configure_lwobject(ob, LC_EUID)", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                configure_lwobject(lwob, LC_EUID, "other");
                return geteuid(lwob) == "other";
            }
        }),
        ({ "lwobject_info(ob, LC_EUID)", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                return lwobject_info(lwob, LC_EUID) == "lwuid";
            }
        }),
        ({ "function_exists()", 0,
            function int()
            {
                return function_exists("push", new_lwobject("/lwo/stack")) == "/lwo/stack";
            }
        }),
        ({ "functionlist()", 0,
            function int()
            {
                return deep_eq(mkmapping(functionlist(new_lwobject("/lwo/stack"))), (["create","push","pop","top","empty"]));
            }
        }),
        ({ "variable_exists()", 0,
            function int()
            {
                return variable_exists("stack", new_lwobject("/lwo/stack")) == "/lwo/stack";
            }
        }),
        ({ "variable_list()", 0,
            function int()
            {
                return deep_eq(variable_list(new_lwobject("/lwo/stack")), ({"stack"}));
            }
        }),
        ({ "include_list()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                return deep_eq(include_list(lwob), ({ "/lwo/stack.c", 0, 0}));
            }
        }),
        ({ "inherit_list()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                return deep_eq(inherit_list(lwob), ({ "/lwo/stack.c"}));
            }
        }),
        ({ "get_ & set_extra_wiz_info()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                set_extra_wizinfo(lwob, "Lightweight!");
                return get_extra_wizinfo(lwob) == "Lightweight!"
                    && get_extra_wizinfo("lwuid") == "Lightweight!";
            }
        }),
        ({ "set_this_object() with last reference", 0,
            function int()
            {
                set_this_object(new_lwobject("/lwo/stack"));
                return lwobjectp(this_object());
            }
        }),
        ({ "bind_lambda() with last reference", 0,
            function int()
            {
                closure cl = bind_lambda(#'this_object, new_lwobject("/lwo/stack"));
                if (!lwobjectp(funcall(cl)))
                    return 0;
                funcall(cl).push("Check");
                return funcall(cl).pop() == "Check";
            }
        }),
        ({ "symbol_function()", 0,
            function int()
            {
                lwobject lwob = new_lwobject("/lwo/stack");
                funcall(symbol_function("push", lwob), "Check");
                return lwob.pop() == "Check";
            }
        }),
        ({ "object_info(OI_DATA_SIZE_TOTAL)", 0,
            function int()
            {
                int old_size = ((var = 0), object_info(this_object(), OI_DATA_SIZE_TOTAL));
                int new_size = ((var = new_lwobject("/lwo/stack")), object_info(this_object(), OI_DATA_SIZE_TOTAL));
                return new_size > old_size;
            }
        }),
        ({ "object_info(OI_NO_LIGHTWEIGHT)", 0,
            function int()
            {
                return object_info(this_object(), OI_NO_LIGHTWEIGHT)
                   && !object_info(find_object("/lwo/stack"), OI_NO_LIGHTWEIGHT);
            }
        }),
        ({ "driver_info(DI_NUM_LWOBJECTS) with lwobjects", 0,
            function int()
            {
                int prev = driver_info(DI_NUM_LWOBJECTS);
                lwobject lwob = new_lwobject("/lwo/stack");

                return driver_info(DI_NUM_LWOBJECTS) == prev + 1;
            }
        }),
        ({ "driver_info(DI_SIZE_LWOBJECTS) with lwobjects", 0,
            function int()
            {
                /* Clear the trace. */
                foreach (int i: 65536)
                    this_object()->noop();

                int prev = driver_info(DI_SIZE_LWOBJECTS);
                lwobject lwob = new_lwobject("/lwo/stack");

                return driver_info(DI_SIZE_LWOBJECTS) > prev;
            }
        }),
        ({ "running internal tests", 0,
            function int()
            {
                msg("\n");
                return new_lwobject("/lwo/tests").run_tests();
            }
        }),
    }),
    function void(int errors) : int finished = &finished
    {
        if(errors)
            shutdown(1);
        else
            start_gc(function void(int error) : int finished = &finished
            {
                if (error)
                    shutdown(1);
                else
                    finished = 1;
            });
        return 0;
    });
}

string *epilog(int eflag)
{
    set_driver_hook(H_CREATE_OB, "create");
    set_driver_hook(H_CREATE_LWOBJECT, "new");
    set_driver_hook(H_CREATE_LWOBJECT_COPY, "copied");
    set_driver_hook(H_CREATE_LWOBJECT_RESTORE, "restored");
    set_driver_hook(H_LWOBJECT_UIDS, unbound_lambda(({}), "lwuid"));

    run_test();
    return 0;
}
