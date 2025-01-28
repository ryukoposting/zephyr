.. zephyr:code-sample:: external_library_rust
   :name: External Rust Library

   Include an external Rust library into the Zephyr build system.

Overview
********

A simple example that demonstrates how to use an external Rust library with
the Zephyr build system.

Experimental Feature
********************

This example uses the experimental `CONFIG_RUST` flag. The Rust subsystem is
unstable, and this example is subject to change significantly as the Rust
subsystem evolves.

Setup
*****

To build this example, Cargo and rustc must be installed on your system.
Rustup is the recommended way to install Cargo and rustc: https://rustup.rs/

Windows Note
************

To use this sample on a Windows host operating system, GNU Make needs to be in
your path. This can be setup using chocolatey or by manually installing it.

Chocolatey Method
=================

Install make using the following command:

.. code-block:: bash

   choco install make

Once installed, build the application as normal.

Manual Install Method
=====================

The pre-built make application can be downloaded from
https://gnuwin32.sourceforge.net/packages/make.htm by either getting both the
``Binaries`` and ``Dependencies`` and extracting them to the same folder, or
getting the ``Complete package`` setup. Once installed and in the path, build
the application as normal.
