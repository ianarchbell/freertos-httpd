Using an RTOS as an implementation technology won't make implementation of FSMs easier. All it will do is add another layer of complexity, with hidden characteristics.

I presume your FSMs are specified in an FSM description language of whatever kind. Statecharts, CSP, and SDL are well-known techniques, but there are many others. If that isn't the case, then you should start there before implementing anything.

It sounds to me as if you would benefit from using alternative design patterns to implement FSMs. If you are using if/then/else or case statements for FSMs with more than half a dozen states/events, then that will definitely be the case. I've seen the results of such cancerous implementation techniques; the result was very ugly and very expensive. People changed job rather than work on it!

Having said that, there are some limited use cases where linear code involving yield() and waitForEvent() can be tractable.

My preference:
each FSM is all in a single thread or a superloop. Multiple FSMs can run in separate threads or sequentially within the superloop
external hardware events cause an interrupt, and ISR mutates them into a message that is put in a queue
FSM sucks one event from that queue and processes it to completion. Rinse and repeat.
FSM can create events, which are also put in the same message queue to be processed by that FSM
FSM cannot distinguish whether an message was from hardware event or software
choose a design pattern so that you can log events/states in real time, and when modification is required it should be obvious which line of code (i.e. state-event pair) needs changing
if appropriate have multiple "worker" threads each of which takes one event from a global queue and processes it to completion. There should be less worker threads than cores
or, of course, just use Hoare's CSP and an embedded multicore processor.