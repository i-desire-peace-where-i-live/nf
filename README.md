# Notefinder is an open-source note-taking and personal information management software implemented in C and GTK+.

<p align="center">
<img src="https://github.com/i-desire-peace-where-i-live/nf/raw/master/images/notefinder.png?raw=true"/>
</p>

Notefinder can be used to keep, organize and sync various bits and pieces of personal information like notes, bookmarks, tasks, contacts, etc. (*entries* from now on).

It's able to retrieve entries from different kinds of *sources*, for example:

* Local text files in a directory
* Bookmarks from your web browser (currently only Firefox is supported)

There are (currently) uncertain plans to add support for more source *backends*, for example:
* Google Keep
* Raindrop
* Remember the Milk, etc.

I aim to make it extensible by users with Perl-written hooks (by embedding Perl within the application, think of it as if it was Lisp in Emacs).

The source code is available under BSD 0 clause license (nearly public domain).
