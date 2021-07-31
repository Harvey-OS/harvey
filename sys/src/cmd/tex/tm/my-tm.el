;; load tm.el if not loaded

(if (fboundp 'make-tm-document)
    ()
  (load "tm"))


(setq tm-face-dir "/usr/gorby/gorby/face")


;; define personal preamble entries for make-tm-document

(defun tm-preamble ()
  "Add to the standard make-tm-document preamble"
    (insert "\\authordata{" (user-login-name) "}\n")
    (insert "%\\chargecase{978899-2000}\n")
    (insert "%\\filecase{47027}\n")
    (insert "%\\documentno{890000}{02}{TMS}\n")
    (insert "\\keywords{}\n")
    (insert "\\mercurycode{CMP}\n")
    (insert "\\date{\\today}\n")
    (insert "\\memotype{TECHNICAL MEMORANDUM}\n")
    (insert "%\\abstract{}\n")
    (insert "\\organizationalapproval\n")
    (insert "\\markproprietary\n")
    (insert "%\\markdraft*\n")
    )

;; define personal postamble entries for make-tm-document
;; ie, stuff after signature but before end{document}

(defun tm-postamble ()
  "Add to the standard make-tm-document postamble"
  (insert "%\\noindent Atts.\\\\\n")
  (insert "%References\\\\\n")
  (insert "%Appendixes A-C\\\\\n")
  (insert "%Table 1\n\n")
  (insert "%\\copyto{{\\nobreak\\topsep=0pt\\partopsep=0pt\\begin{tabbing}\\nobreak%\n")
  (insert "%\\hskip2in\\=\\kill%\n")
  (insert "%All Members Department 59114\\\\\n")
  (insert "%All Members Department 59112\\\\\n")
  (insert "%All Supervision Center 5911\\\\\n")
  (insert "%All Members MSTC\\\\\n")
  (insert "%S.\\ G.\\ Chappell	\\>LC 4N-E21\\\\\n")
  (insert "%J.\\ W.\\ Hunt		\\>LC 4N-F15\\\\\n")
  (insert "%D.\\ G.\\ Belanger	\\>MH 3C-526A\\\\\n")
  (insert "%J.\\ F.\\ Maranzano	\\>LC 4N-G16\\\\\n")
  (insert "%D.\\ R.\\ Ryan		\\>HR 2C-194\n")
  (insert "%\\end{tabbing}}}\n")
  (insert "%\\bibliography{sim,rapid,tutorial,softengr}\n")
  (insert "%\\appendices\n")
  (insert "%\\input{glossary}\n")
  (insert "%\\tableofcontents\n")
  (insert "%\\listoffigures\n")
  (insert "%\\coversheet\n")
  )

;; define personal preamble entries for make-attletter-document

(defun attletter-preamble ()
  "Add to the standard make-tm-document preamble"
    (insert "\\makelabels\n")
    (insert "\\phone{201 580-4428}\\room{4N-E01}\n")
    (insert "\\eaddress{tla\\%bartok@RESEARCH.ATT.COM}\n")
    (insert "\\facesignature{Terry L Anderson\\\\Software Architecture Department}{/usr/gorby/gorby/face/tla/face}\n")
    )

