libev-git
=========

libev is a high-performance event loop/event model with lots of features.
(see benchmarks at http://libev.schmorp.de/bench.html)

libev-git is a fork of libev with three simple goals:
1. Maintain strict compatibility with upstream libev. That means no patches to
   the libev source code that are not submitted back to the libev project.
   Modifications to maintainer scripts, build scripts, and documentation, on
   the other hand, may or may not be upstreammed.
2. Support out-of-tree builds. This makes building for multiple platforms
   simultaneously easier than it was with autotools. I did this by rewriting
   the build system in CMake.
3. Migrate to a modern version control system. As of this writing, upstream
   libev is managed in CVS, which is pretty intolerable. As its name implies,
   libev-git is managed in Git instead, with the full version control history
   rescued from the upstream CVS repository.

BUILDING
--------

Use the following commands to configure, build, and install libev. This is the
simplest case. Take a look at the top-level `CMakeLists.txt` for more options.

```shell
$ cmake .
$ make
$ sudo make install
```

ABOUT
-----

* [Homepage](http://software.schmorp.de/pkg/libev.html)
* [Mailinglist Address](mailto:libev@lists.schmorp.de)
* [Mailinglist Archive](http://lists.schmorp.de/cgi-bin/mailman/listinfo/libev)
* [Library Documentation](http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod)

Libev is modeled (very loosely) after [libevent](https://github.com/libevent/libevent)
and [the Event perl module](http://search.cpan.org/perldoc?Event), but it is
faster, scales better, and is more correct... and more featureful... and
smaller.

Some of the specialties of libev not commonly found elsewhere are:
* extensive and detailed, readable documentation (not doxygen garbage).
* fully supports fork, can detect fork in various ways and automatically
  re-arms kernel mechanisms that do not support fork.
* highly optimised select, poll, epoll, kqueue and event ports backends.
* filesystem object (path) watching (with optional linux inotify support).
* wallclock-based times (using absolute time, cron-like).
* relative timers/timeouts (handle time jumps).
* fast intra-thread communication between multiple
   event loops (with optional fast linux eventfd backend).
* extremely easy to embed (fully documented, no dependencies,
  autoconf supported but optional).
* very small codebase, no bloated library, simple code.
* fully extensible by being able to plug into the event loop,
  integrate other event loops, integrate other event loop users.
* very little memory use (small watchers, small event loop data).
* optional C++ interface allowing method and function callbacks
  at no extra memory or runtime overhead.
* optional Perl interface with similar characteristics (capable
  of running Glib/Gtk2 on libev).
* support for other languages (multiple C++ interfaces, D, Ruby,
  Python) available from third-parties.

Examples of programs that embed libev: the EV perl module, node.js, auditd,
rxvt-unicode, gvpe (GNU Virtual Private Ethernet), the
[Deliantra MMORPG server](http://www.deliantra.net/), Rubinius
(a next-generation Ruby VM), the Ebb web server, the Rev event toolkit.

CONTRIBUTORS
------------

libev was written and designed by Marc Lehmann and Emanuele Giaquinta.

The following people sent in patches or made other noteworthy contributions to
the design (for minor patches, see the Changes file. If I forgot to include
you, please shout at me, it was an accident):

* W.C.A. Wijngaards
* Christopher Layne
* Chris Brody
