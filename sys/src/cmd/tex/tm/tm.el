;; load latex.el if not loaded

(if (boundp 'LaTeX-standard-styles)
    ()
  (load "latex"))

;; append tm related standard-options

(setq LaTeX-standard-options (append LaTeX-standard-options '( 
        ("authors")              ; tla
	("bitmap")               ; tla
	("dingbat")              ; tla
	("expicture")            ; tla
	)))


;; append tm related standard-styles

(setq LaTeX-standard-styles (append  LaTeX-standard-styles '(
	("attletter")                   ;tla's att letterhead letter
	("rp")                          ;tla's released paper
	("tm")                          ;tla's tech memo style
	)))



;; makes a template for a new TM

(defun make-tm-document (commandp)
  "Select LaTeX-mode, then insert a template for a new TM document,
prompting for options.  With an argument, some additional
helpful comments are inserted.  Any previous contents of the buffer
are killed (and saved on the kill-ring).

The default template header can be replaced by a customized one
inserted by the function make-LaTeX-document-hook which is run with
point at the beginning of the buffer."

  (interactive "p")

  (let (LaTeX-option loop)
    (if (eq major-mode 'LaTeX-mode)
	(LaTeX-mode))
    (setq loop t)
    (setq LaTeX-option-list "")
    (while loop (setq LaTeX-option (completing-read
				     "Document option (RET when done): "
				     LaTeX-standard-options
				     nil nil nil))
	   (if (equal LaTeX-option "")
	       (setq loop nil)
	     (setq LaTeX-option-list (format "%s,%s"
					     LaTeX-option-list
					     LaTeX-option))))
    (if (not (eql 0 (length LaTeX-option-list)))
	(setq LaTeX-option-list
	      (substring LaTeX-option-list 1)))	; discard leading ","
    (setq tm-title (read-string "Document Title: " ))
    (kill-region (point-min) (point-max))
    (goto-char (point-min))
    (if (boundp 'make-LaTeX-document-hook)
	(run-hooks 'make-LaTeX-document-hook)
      (progn
	(insert "% -*-latex-*-\n")	;must be lower-case for autoload
	(goto-char (point-max))		;on find-file to work
	(insert "% Document name: "
		(if (null buffer-file-name)
		    "unknown"
		  buffer-file-name)
		"\n")
	(insert "% Creator: " (user-full-name)
		" [" (user-login-name) "@" (system-name) "]\n")
	(insert "% Creation Date: " (current-time-string))
	(if (null current-prefix-arg)
	    (insert "\n")
	  (insert "
%------------------------------------------------------------------------
% EVERYTHING TO THE RIGHT OF A  %  IS A REMARK TO YOU AND IS IGNORED BY
% LaTeX.
%
% WARNING!  DO NOT TYPE ANY OF THE FOLLOWING 10 CHARACTERS AS NORMAL TEXT
% CHARACTERS:
%                &   $   #   %   _    {   }   ^   ~   \\
%
% The following seven are printed by typing a backslash in front of them:
%  \$  \&  \#  \%  \_  \{  and  \}.
%------------------------------------------------------------------------
"))
	))
    (goto-char (point-max))
    (if (not (equal (buffer-substring (1- (point-max)) (point-max)) "\n"))
	(insert "\n"))
    (insert "\\documentstyle[" LaTeX-option-list "]{tm}\n")
    (insert "\\newcommand{" LaTeX-index-macro "}[1]{{#1}\\index{{#1}}}\n")
    (insert "\\title{" tm-title "}\n")
    (if (fboundp 'tm-preamble)
	(tm-preamble))
    (insert "\\begin{document}\n")
    (insert "\\bibliographystyle{atttm}\n")
    (insert "\\makehead\n")
    (save-excursion
      (insert "\n\\makefacesignature{") 
      (if (eq tm-face-dir nil)
	  ()
	(insert tm-face-dir)
	)
      (insert "}\n")
      (if (fboundp 'tm-postamble)
	  (tm-postamble))
      (insert "\\end{document}\n")
      )
    )
  )

;; makes a template for a new AT&T letter

(defun make-attletter-document (commandp)
  "Select LaTeX-mode, then insert a template for a new attletter
prompting for options.  With an argument, some additional
helpful comments are inserted.  Any previous contents of the buffer
are killed (and saved on the kill-ring).

The default template header can be replaced by a customized one
inserted by the function make-LaTeX-document-hook which is run with
point at the beginning of the buffer."

  (interactive "p")

  (let (LaTeX-option loop)
    (if (eq major-mode 'LaTeX-mode)
	(LaTeX-mode))
    (setq loop t)
    (setq LaTeX-option-list "")
    (while loop (setq LaTeX-option (completing-read
				     "Document option (RET when done): "
				     LaTeX-standard-options
				     nil nil nil))
	   (if (equal LaTeX-option "")
	       (setq loop nil)
	     (setq LaTeX-option-list (format "%s,%s"
					     LaTeX-option-list
					     LaTeX-option))))
    (if (not (eql 0 (length LaTeX-option-list)))
	(setq LaTeX-option-list
	      (substring LaTeX-option-list 1)))	; discard leading ","
    (setq to-address (read-string "To Address: " ))
    (setq dear-who (read-string "Salutation: " ))
    (kill-region (point-min) (point-max))
    (goto-char (point-min))
    (if (boundp 'make-LaTeX-document-hook)
	(run-hooks 'make-LaTeX-document-hook)
      (progn
	(insert "% -*-latex-*-\n")	;must be lower-case for autoload
	(goto-char (point-max))		;on find-file to work
	(insert "% Document name: "
		(if (null buffer-file-name)
		    "unknown"
		  buffer-file-name)
		"\n")
	(insert "% Creator: " (user-full-name)
		" [" (user-login-name) "@" (system-name) "]\n")
	(insert "% Creation Date: " (current-time-string))
	(if (null current-prefix-arg)
	    (insert "\n")
	  (insert "
%------------------------------------------------------------------------
% EVERYTHING TO THE RIGHT OF A  %  IS A REMARK TO YOU AND IS IGNORED BY
% LaTeX.
%
% WARNING!  DO NOT TYPE ANY OF THE FOLLOWING 10 CHARACTERS AS NORMAL TEXT
% CHARACTERS:
%                &   $   #   %   _    {   }   ^   ~   \\
%
% The following seven are printed by typing a backslash in front of them:
%  \$  \&  \#  \%  \_  \{  and  \}.
%------------------------------------------------------------------------
"))
	))
    (goto-char (point-max))
    (if (not (equal (buffer-substring (1- (point-max)) (point-max)) "\n"))
	(insert "\n"))
    (insert "\\documentstyle[" LaTeX-option-list "]{attletter}\n")
    (insert "\\newcommand{" LaTeX-index-macro "}[1]{{#1}\\index{{#1}}}\n")
    (insert "\\signature{" (user-full-name) "}\n")
    (if (fboundp 'attletter-preamble)
	(attletter-preamble))
    (insert "\\begin{document}\n")
    (insert "\\begin{letter}{" to-address "}\n")
    (insert "\\opening{" dear-who "}\n")
    (save-excursion
      (if (fboundp 'attletter-postamble)
	  (tm-postamble))
      (insert "\\closing{Sincerely,}\n")
      (insert "\\end{letter}\n")
      (insert "\\end{document}\n")
      )
    )
  )


;; define a key for make-tm-document

(if LaTeX-mode-map
  (let ((map LaTeX-mode-map))		;if defined add
    (define-key map "\C-ct" 'make-tm-document) ; tla Tech Memo
    (setq LaTeX-mode-map map)))

