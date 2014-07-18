%%
%% This is file `bluenote.cls',
%% generated with the docstrip utility.
%%
%% The original source files were:
%%
%% bluenote.dtx (with options: `cls')
%%
\NeedsTeXFormat{LaTeX2e}[1999/12/01]
\ProvidesClass{bluenote}[2012/07/20 v0.2 Blue notes]
\DeclareOption*{\PassOptionsToPackage{\CurrentOption}{article}}
\ProcessOptions
\LoadClass{article}
\RequirePackage{a4wide}
\RequirePackage{url}
\RequirePackage{xspace}
\RequirePackage{paralist}
\RequirePackage{draftwatermark}
\SetWatermarkScale{8}
\SetWatermarkAngle{50}
\SetWatermarkText{Blue Note}
\SetWatermarkFontSize{3cm}
\newcommand\blueProject[1]{\def\blue@project{#1}}
\blueProject{KWARC}
\newif\ifblueURI\blueURItrue
\newcommand\noblueURI{\blueURIfalse}
\newcommand\blueURI{\urldef{\blue@URI}\url}
\blueURI{http://kwarc.info}
\def\title#1{\gdef\@title{--- {\blue@project} Blue Note\thanks{Inspired by the ``blue book''
      in Alan Bundy's group at the University of Edinburgh, {\blue@project} blue notes,
      are documents used for fixing and discussing $\epsilon$-baked ideas in projects by
      the {\blue@project} group\ifblueURI\ (see \blue@URI)\fi. Unless specified otherwise, they are for
      project-internal discussions only. Please only distribute outside the
      {\blue@project} group after consultation with the author.}\quad ---\\ #1}}
\endinput
%%
%% End of file `bluenote.cls'.