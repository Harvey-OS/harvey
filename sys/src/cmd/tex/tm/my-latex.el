;; load latex if not loaded

(if (boundp 'LaTeX-standard-styles)
    ()
  (load "latex"))

(defvar TeX-default-mode 'latex-mode
  "*Mode to enter for a new file when it can't be determined whether
the file is plain TeX or LaTeX or what.")
(defun set-ispell-for-TeX () 
  "setup ispell hooks for TeX mode"
  (progn
 (setq ispell-filter-hook "delatex")
  (setq ispell-filter-hook-args '())))
(defvar TeX-mode-hook 'set-ispell-for-TeX 
   "set ispell hooks on entry to mode")

(defun set-ispell-for-nroff () 
  "setup ispell hooks for nroff mode"
  (progn
  (setq ispell-filter-hook "/usr/bin/deroff")
  (setq ispell-filter-hook-args '("-w"))))
(defvar nroff-mode-hook 'set-ispell-for-nroff 
   "set ispell hooks on entry to mode")
