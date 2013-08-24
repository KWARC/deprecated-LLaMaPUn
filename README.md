<img src="doc/transparent_llama_sir.png" alt="LLaMaPUn Logo" width="400"/>

The **LLaMaPUn** library consists of a wide range of processing tools for natural language and mathematics.

---

**Disclaimer:** This Github repository is currently undergoing gradual migration from the original [subversion repository](https://svn.kwarc.info/repos/lamapun/lib/).
The migration consists of reorganizing the libraries, and preparing a CPAN-near bundle including a testbed and detailed documentation.
This process also brings a namespace change to the now properly spelled LLaMaPUn.

Several upcoming deployments of the [CorTeX framework](https://github.com/dginev/CorTeX) have motivated the move to GitHub
and provide an outlook for a number of fixes and features to be added to the library.

## Overview
 * **LLaMaPUn::Preprocessor**
   
   Preprocessing module carrying a sane transition from LaTeXML ".tex.xml|.noparse.xml" documents to plain text.

 * **LLaMaPUn::Preprocessor::Purify**
 
   Purification of Math and Textual modality in XML documents created via LaTeXML.

 * **LLaMaPUn::Preprocessor::MarkTokens**
 
   In-place tokenization for LaTeXML XML documents, marks sentences, words, formulas and tokens.

 * **LLaMaPUn::Tokenizer**
 
   Tokenization module for LaMaPUn normalized documents (plain text)

 * **LLaMaPUn::LaTeXML**
   
   Convenience API, building on top of the LaTeXML libraries.
   Also adds support for a TEI-near XHTML conversion.

 * More to be migrated...

## Externals
 * 
 
## See also
 * The official [project page](https://trac.kwarc.info/lamapun/) at Jacobs University 
 * The kick-off [LaMaPUn paper](http://www.kwarc.info/projects/lamapun/pubs/AST09_LaMaPUn+appendix.pdf) from 2009.
 
## Contact
Feel free to send any feedback to the project maintainer at d.ginev@jacobs-university.de

---

![A LLaMa PUn](doc/a_llama_pun.jpg)
