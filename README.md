ISBN2node 1.4 for Backdrop 1.x
=========

ISBN2node enables you to create nodes in a book archive based on ISBN's.
ISBN's or International Standard Book Numbers are unique identifiers for
books, and based on ISBN you can gather quite a bit of information about
a book as well as find its cover. The module will allow users to easily
build archives of physical books based on these book's ISBN's.

The module creates a new content type called ISBN2node-book, which can
be configured.


Required Modules
------------

This module requires that the following modules are also enabled:

- Node Backdrop core module
- Image Backdrop core module


Installation
------------

- Install this module using the official Backdrop CMS instructions at
  https://docs.backdropcms.org/documentation/extend-with-modules.

- Visit the configuration page under Administration > Configuration > Content >
  ISBN2node (admin/config/content/isbn2node) and select your preferred
  ISBN DB. If you select ISBNdb, you will need to acquire an access key.

- (Optional) You may choose to configure the content type created by this
  module at Administration > Structure > Content types > ISBN2node-book
  (admin/structure/types/manage/isbn2node_book). From here you can change
  the name of the new content type, add fields, etc.


Documentation
-------------

Additional documentation is located in the Wiki:
https://github.com/backdrop-contrib/isbn2node/wiki/Documentation.


Issues
------

Bugs and Feature Requests should be reported in the Issue Queue:
https://github.com/backdrop-contrib/mymodule/issues.


Current Maintainers
-------------------

- [leeksoup](https://github.com/leeksoup)


Credits
-------

- Ported to Backdrop CMS by [leeksoup](https://github.com/leeksoup).
- Originally written for Drupal by [Martin Joergensen](https://www.drupal.org/u/vertikaldk).


License
-------

This project is GPL v2 software.
See the LICENSE.txt file in this directory for complete text.
