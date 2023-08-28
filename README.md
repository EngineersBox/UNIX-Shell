# COMP3300/COMP6330 Assignment1: Implementing A Unix Shell

## Problem Description

For this assignment, you will implement a basic shell in linux, called the ANU Basic Interactive Shell (ANUbis). This assignment is adapted from a project specification designed by the authors of OSTEP. 

See the directory [anubis/](anubis) for the detailed specification and test cases.

For the details of submission deadline, please refer to the Wattle site for this course.  

# Submission instructions

Submission will be primarily through Gitlab for this assignment. 

**PLEASE READ THE FOLLOWING CAREFULLY**

## To get started
- Fork this repository to your own namespace. 
    * **Make sure** that the 'visibility' of your fork is set to **private**.
    * **Make sure** that you select _your_ namespace. (this is only applicable to students who have greater than normal Gitlab access - others will only be able to select their own namespace.)
    * **DO NOT RENAME THE PROJECT - LEAVE THE NAME AND URL SLUG EXACTLY AS IS**
    * You may notice an additional member in your project - `comp3300-2023-s2-marker`. **Do not remove this member from your project**.
- Clone the repository to your virtual machine. You most likely will have to use HTTPS for this as SSH is unavailable to most connections.
- Get started!
  - The specification of the problem is contained within the folder [anubis/](./anubis/), and has its own README.md file. 
  - You can view rendered markdown using the Gitlab editor, or by opening the folder in VSCode.

## Working

- Make sure to commit and push to the Gitlab regularly to save your work.

- Test your program manually first as you build up your implementation.

- You can check the completeness of your implementation (as far as this assignment is concerned) against the test cases we provided in the [anubis/tests](./anubis/tests) directory. You don't normally run your program directly on these test case; rather you use the provided script [anubis/test-anubis.sh](./anubis/test-anubis.sh) to run the tests. There are a total of 30 test cases, testing all aspects of your implementation against the specification.  

- This gitlab repo uses a CI pipeline that will run the test cases on your code everytime you push your changes to the repository. While this is reasonably accurate (as far as we can test), it might misclassified your program output as wrong against a particular test case. If you noticed that the result of the Gitlab CI pipeline differs from the `test-anubis.sh` script, please note that down in your report and we will manually test your code. 

-  You may add and write your report source document (`.docx`, `.md`, whatever) to this repository if you wish

- Add your report PDF to this repository with the name "uXXXXXXX_report.pdf" where "uXXXXXXX" is your UID. **You will still have to submit your PDF report to Turnitin**.


## Submission

- Make sure your **latest** report PDF has been uploaded to turnitin. **This is important, do not forget this.**
- Make sure your work is **committed and pushed** to this repository **before** the deadline (accounting for extensions/EAPs). 
    - As per the usual, a 100% late submission deduction applies.

- Once your work has been pushed to your Gitlab, you do not need to do anything further! We are able to fork your submissions for marking.
  * You can double check your submission status by going to your fork of the assignment and checking for your most recent commit.
  * Don't be afraid to ask questions about the process in the labs or on EdStem!

- The submission time on gitlab will be used to determine whether you submitted on time. However, your mark will not be finalised until you have also submitted your report to Turnitin for validation. 

- Your artefact must be implemented in the C programming language. 

- You must implement all the required functionalities using only standard libraries in C (and procps), using the provided template file (`anubis.c`). The libraries "included" in the template `anubis.c` are sufficient to implement your code. 

## Statement of originality

- You must include a Statement of Originality in your report. Use the following template for the statement:

    >   I declare that everything I have submitted in this assignment is entirely my
  own work, with the following exceptions:
    >  - list the sources of code you used, articles, blogs, books etc, if any
    >  - discussions with others related to this assignment, if any 

- Please keep in mind that this is an individual assignment, so be aware that collaborating with others to solve this assignment may constitute an academic misconduct. 

- Read the [ANU page on academic integrity](https://www.anu.edu.au/students/academic-skills/academic-integrity) for more information on academic integrity. 


## Assessment

Your assignment is assessed based on your artefact (i.e., the code you submitted) and your report, using the following rubrics. 

- Artefact [50%]: 
    * [30%] Pass automated test cases.
    * [20%] Code quality: 
        - clearly formatted and commented
        - compiled without errors or warnings
        - code "hygiene": avoid hardcoded constants, minimise memory leaks, checks for runtime errors (null pointers, out-of-bound accesses, etc).  

- Report [50%]: 
    * Completeness: all required functionalities explained.
    * Correctness: explanation of algorithm(s)/data structure(s) used is correct. 
    * Writing quality: 
        - Your writing should be concise, readable and comprehensible. Focus on important parts of your code and explain key ideas in the code, e.g., your overall parsing strategy, why a particular system call is needed, the subtleties in the order of input/output redirections, etc. Don't waste space by explaining every line of code -- for that you can use comments in the source file. 
        - Your report should be professionally typeset, so for example, avoid handwritten notes, and if you use screen captures, they must be readable. 
        - Your report should contain proper acknowledgement/citations if you use any external sources for your ideas. 
    * Report length <= 1500 words, excluding citations, figures, code excerpts, tables. 

- Penalty: 
    
    * **No late submission allowed** without a prior approval from the convenor. Late submission penalty is 100%. 

    * Using unapproved libraries or functions may result in 100% penalty, depending on the severity of the violation. It may also lead to academic misconduct investigations. 

## Acknowledgement

The assignment specification and its automated test cases and test scripts are adapted from [the "shell" OSTEP project](https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/processes-shell) by Remzi Arpaci-Dusseau.
