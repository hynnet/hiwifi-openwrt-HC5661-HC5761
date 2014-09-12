;;; quilt.el v0.45.3 - a minor mode for working with files in quilt
;;; http://stakeuchi.sakura.ne.jp/dev/quilt-el
;;;
;;; Copyright 2005  Matt Mackall <mpm@selenic.com>
;;;
;;; Satoru Takeuchi<nqm08501@nifty.com> took over this package
;;; from Matt Mackall.
;;;
;;; This software may be used and distributed according to the terms
;;; of the GNU General Public License, incorporated herein by reference.
;;;
;;; Usage: add (load "~/quilt.el") to your .emacs file

(defun quilt-buffer-file-name-safe ()
  (let ((fn buffer-file-name))
    (if (and fn (file-exists-p fn))
	fn)))

(defun quilt-bottom-p ()
  (if (> (call-process "quilt" nil nil nil "applied") 0) 1))

(defun quilt-patches-directory ()
  (or (getenv "QUILT_PATCHES")
      "patches"))

(defun quilt-find-dir (fn)
  "find the top level dir for quilt from fn"
  (if (or (not fn) (equal fn "/"))
      nil
    (let ((d (file-name-directory fn)))
      (if (file-accessible-directory-p (concat d "/.pc"))
	  d
	(quilt-find-dir (directory-file-name d))))))

(defun quilt-dir (&optional fn)
  (quilt-find-dir (if fn fn
		    (let ((fn2 (quilt-buffer-file-name-safe)))
		      (if fn2 fn2
			(expand-file-name default-directory))))))

(defun quilt-drop-dir (fn)
  (let ((d (quilt-find-dir fn)))
    (substring fn (length d) (length fn))))

(defun quilt-p (&optional fn)
  "check if the given file or current buffer is in a quilt tree"
  (if (quilt-dir fn) 't nil))

(defun quilt-save ()
  (save-some-buffers nil 'quilt-p))

(defun quilt-owned-p (fn)
  "check if the current buffer is quilt controlled"
  (if (not fn)
      nil
    (let ((pd (file-name-nondirectory
		(directory-file-name (file-name-directory fn)))))
      (and
       (not (string-match "\\(~$\\|\\.rej$\\)" fn))
       (not (equal pd (quilt-patches-directory)))
       (not (equal pd ".pc"))
       (quilt-p fn)))))

(defun quilt-cmd (cmd &optional buf)
  "execute a quilt command at the top of the quilt tree for the given buffer"
  (let ((d default-directory)
	 (qd (quilt-dir)))
    (if (not qd)
	(shell-command (concat "quilt " cmd) buf)
      (cd qd)
      (shell-command (concat "quilt " cmd) buf)
      (cd d))))

(defun quilt-cmd-to-string (cmd)
  "execute a quilt command at the top of the quilt tree for the given buffer"
  (let ((d default-directory)
	 (qd (quilt-dir)))
    (if (not qd)
	nil
      (cd qd)
      (let ((r (shell-command-to-string (concat "quilt " cmd))))
	(cd d) r))))

(defun quilt-applied-list ()
  (let ((s (quilt-cmd-to-string "applied")))
    (if s
	(split-string s "\n"))))

(defun quilt-file-list ()
  (let ((s (quilt-cmd-to-string "files")))
    (if s
	(split-string s "\n"))))

(defun quilt-patch-list ()
  (let ((s (quilt-cmd-to-string "series")))
     (if s
	 (split-string s "\n"))))

(defun quilt-top-patch ()
  (if (quilt-bottom-p)
      nil
    (file-name-nondirectory
     (substring (quilt-cmd-to-string "top")  0 -1))))

(defun quilt-complete-list (p l)
  (defun to-alist (list n)
    (if list
	(cons (cons (car list) n)
	      (to-alist (cdr list) (+ 1 n)))
      nil))
  (completing-read p (to-alist l 0) nil t))

(defun quilt-editable (f)
  (let ((qd (quilt-dir))
	 (fn (quilt-drop-dir f)))
    (defun editable (file dirs)
      (if (car dirs)
	  (if (file-exists-p (concat qd ".pc/" (car dirs) "/" file))
	      't
	    (editable file (cdr dirs)))
	nil))
    (if qd
	(if (quilt-bottom-p)
	    (quilt-cmd "applied")	; to print error message
	  (editable fn (if quilt-edit-top-only
			   (list (quilt-top-patch))
			 (cdr (cdr (directory-files (concat qd ".pc/"))))))))))

(defun quilt-short-patchname ()
  (let ((p (quilt-top-patch)))
    (if (not p)
	"none"
      (let ((p2 (file-name-sans-extension p)))
	   (if (< (length p2) 10)
	       p2
	     (concat (substring p2 0 8) ".."))))))

(defvar quilt-mode-line nil)
(make-variable-buffer-local 'quilt-mode-line)

(defun quilt-update-modeline ()
  (interactive)
  (setq quilt-mode-line
	(concat " Q:" (quilt-short-patchname)))
  (force-mode-line-update))

(defun quilt-revert ()
  (defun revert-or-hook-buffer ()
    ;; If the file doesn't exist on disk it can't be reverted, but we
    ;; need the revert hooks to run anyway so that the buffer's
    ;; editability will update.
    (if (file-exists-p buffer-file-name)
	(revert-buffer 't 't)
      (run-hooks 'after-revert-hook)))
  (defun revert (buf)
    (save-excursion
      (set-buffer buf)
      (let* ((fn (quilt-buffer-file-name-safe)))
	(if (quilt-p fn)
	  (quilt-update-modeline))
	(if (and (quilt-owned-p fn)
		 (not (buffer-modified-p)))
	  (revert-or-hook-buffer)))))
  (defun revert-list (buffers)
    (if (not (car buffers))
	nil
      (revert (car buffers))
      (revert-list (cdr buffers))))
  (revert-list (buffer-list)))

(defun quilt-push (arg)
  "Push next patch, force with prefix arg"
  (interactive "P")
  (quilt-save)
  (if arg
      (quilt-cmd "push -f" "*quilt*")
    (quilt-cmd "push -q"))
  (quilt-revert))

(defun quilt-pop (arg)
  "Pop top patch, force with prefix arg"
  (interactive "P")
  (quilt-save)
  (if arg
      (quilt-cmd "pop -f")
    (quilt-cmd "pop -q"))
  (quilt-revert))

(defun quilt-push-all (arg)
  "Push all remaining patches"
  (interactive "P")
  (quilt-save)
  (if arg
      (quilt-cmd "push -f" "*quilt*")
    (quilt-cmd "push -qa"))
  (quilt-revert))

(defun quilt-pop-all (arg)
  "Pop all applied patches, force with prefix arg"
  (interactive "P")
  (quilt-save)
  (if arg
      (quilt-cmd "pop -af")
    (quilt-cmd "pop -qa"))
  (quilt-revert))

(defun quilt-goto ()
  "Go to a specified patch"
  (interactive)
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (let ((arg (quilt-complete-list "Goto patch: " (quilt-patch-list))))
	(if (string-equal arg "")
	    (message "no patch name is supplied")
	    (quilt-save)
	    (if (file-exists-p (concat qd ".pc/" arg))
		(quilt-cmd (concat "pop -q " arg) "*quilt*")
	      (quilt-cmd (concat "push -q " arg) "*quilt*"))
	    (quilt-revert))))))

(defun quilt-top ()
  "Display topmost patch"
  (interactive)
  (quilt-cmd "top"))

(defun quilt-find-file ()
  "Find a file in the topmost patch"
  (interactive)
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (if (quilt-bottom-p)
	  (quilt-cmd "applied")		; to print error message
	(let ((l (quilt-file-list)))
	  (if (not l)
	      (message "no file is existed in this patch")
	    (let ((f (quilt-complete-list "File: " l)))
	      (if (string-equal f "")
		  (message "file name is not specified")
		(find-file (concat qd f))))))))))

(defun quilt-files ()
  "Display files in topmost patch"
  (interactive)
  (quilt-cmd "files"))

(defun quilt-import (fn pn)
  "Import external patch"
  (interactive "fPatch to import: \nsPatch name: ")
  (if (not pn)
      (message "no patch name supplied")
    (quilt-cmd (concat "import -p " pn ".patch " (if fn fn pn)))))

(defun quilt-diff ()
  "Display diff of current changes"
  (interactive)
  (quilt-save)
  (quilt-cmd "diff" "*diff*"))

(defun quilt-new (f)
  "Create a new patch"
  (interactive "sPatch name: ")
  (if (string-equal f "")
      (message "no patch name is supplied")
    (quilt-save)
    (quilt-cmd (concat "new " f ".patch"))
    (quilt-revert)))

(defun quilt-applied ()
  "Show applied patches"
  (interactive)
  (quilt-cmd "applied" "*quilt*"))

(defun quilt-add (arg)
  "Add a file to the current patch"
  (interactive "b")
  (save-excursion
    (set-buffer arg)
    (let ((fn (quilt-buffer-file-name-safe)))
      (cond
       ((not fn)
	(message "buffer %s is not associated with any buffer" (buffer-name)))
       ((not (quilt-dir))
	(quilt-cmd (concat "push")))	; to print error message
       (t
	(quilt-cmd (concat "add " (quilt-drop-dir fn)))
	(quilt-revert))))))

(defun quilt-edit-patch ()
  "Edit the topmost patch"
  (interactive)
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (let ((patch (quilt-top-patch)))
	(if (not patch)
	    (quilt-cmd "applied")	; to print error message
	  (quilt-save)
	  (let ((pf (concat qd
			    (format "/%s/" (quilt-patches-directory))
			    patch)))
	    (if (file-exists-p pf)
		(progn (find-file pf)
		       (toggle-read-only))
	      (message (format "%s doesn't exist yet." pf)))))))))

(defun quilt-patches ()
  "Show which patches modify the current buffer"
  (interactive)
  (let ((fn (quilt-buffer-file-name-safe)))
    (cond
     ((not fn)
      (message "buffer %s is not associated with any buffer" (buffer-name)))
     ((not (quilt-dir))
      (quilt-cmd "push"))		; to print error message
     (t
      (quilt-cmd (concat "patches " (quilt-drop-dir fn)))))))

(defun quilt-unapplied ()
  "Display unapplied patch list"
  (interactive)
  (quilt-cmd "unapplied" "*quilt*"))

(defun quilt-refresh ()
  "Refresh the current patch"
  (interactive)
  (quilt-save)
  (quilt-cmd "refresh"))

(defun quilt-remove ()
  "Remove a file from the current patch and revert it"
  (interactive)
  (let ((f (quilt-buffer-file-name-safe)))
    (cond
     ((not f)
      (message "buffer %s is not associated with any patch" (buffer-name)))
     ((not (quilt-dir))
      (quilt-cmd "push")) ; to print error message
     (t
      (if (quilt-bottom-p)
	  (quilt-cmd "applied")		; to print error message
	(let ((dropped (quilt-drop-dir f)))
	  (if (y-or-n-p (format "Really drop %s? " dropped))
	      (progn
		(quilt-cmd (concat "remove " dropped))
		(quilt-revert)))))))))

(defun quilt-edit-series ()
  "Edit the patch series file"
  (interactive)
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (let ((series (concat qd
			     (format "/%s/series" (quilt-patches-directory)))))
	(if (file-exists-p series)
	    (find-file series)
	  (message (quilt-top-patch)))))))

(defun quilt-header (arg)
  "Print the header of a patch"
  (interactive "P")
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (if (not arg)
	  (quilt-cmd "header")
	(let ((p (quilt-complete-list "Patch: " (quilt-patch-list))))
	  (if (string-equal p "")
	      (message "no patch name is supplied")
	    (quilt-cmd (concat "header " p))))))))

(defun quilt-delete (arg)
  "Delete a patch from the series file"
  (interactive "P")
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (let ((p (if arg
		   (quilt-complete-list "Delete patch: " (quilt-patch-list))
		 (quilt-top-patch))))
	(if (string-equal p "")
	    (message "no patch name is supplied")
	  (if (not p)
	      (quilt-cmd "applied")	; to print error message
	    (if (y-or-n-p (format "Really delete %s?" p))
		(progn
		  (quilt-save)
		  (quilt-cmd (concat "delete " p))
		  (quilt-revert)))))))))

(defun quilt-header-commit ()
  "commit to change patch header"
  (interactive)
  (let ((tmp (make-temp-file "quilt-header-")))
    (set-visited-file-name tmp)
    (basic-save-buffer)
    (cd quilt-header-directory)
    (shell-command (concat "EDITOR=cat quilt -r header <" tmp))
    (kill-buffer (current-buffer))
    (delete-file tmp)))

(defvar quilt-header-mode-map (make-keymap))
(define-key quilt-header-mode-map "\C-c\C-c" 'quilt-header-commit)

(defun quilt-edit-header (arg)
  "Edit the header of a patch"
  (interactive "P")
  (let ((qd (quilt-dir)))
    (if (not qd)
	(quilt-cmd "push")		; to print error message
      (let ((p (if arg
		   (quilt-complete-list "Edit patch: " (quilt-patch-list))
		 (quilt-top-patch))))
	  (if (string-equal p "")
	      (message "no patch name is supplied")
	    (if (not p)
		(quilt-cmd "applied")	; to print error message
	      (let ((qb (get-buffer-create (format " *quilt-heaer(%s)*" p))))
		(switch-to-buffer-other-window qb)
		(erase-buffer)
		(kill-all-local-variables)
		(make-local-variable 'quilt-header-directory)
		(setq quilt-header-directory default-directory)
		(setq mode-map "quilt-header")
		(use-local-map quilt-header-mode-map)
		(setq major-mode 'quilt-header-mode)
		(call-process "quilt" nil qb nil "header" p)
		(goto-char 0))))))))

(defun quilt-series (arg)
  "Show patche series."
  (interactive "P")
  (if arg
      (quilt-cmd "series -v")
    (quilt-cmd "series")))

(defvar quilt-mode-map (make-sparse-keymap))
(define-key quilt-mode-map "\C-c.t" 'quilt-top)
(define-key quilt-mode-map "\C-c.f" 'quilt-find-file)
(define-key quilt-mode-map "\C-c.F" 'quilt-files)
(define-key quilt-mode-map "\C-c.d" 'quilt-diff)
(define-key quilt-mode-map "\C-c.p" 'quilt-push)
(define-key quilt-mode-map "\C-c.o" 'quilt-pop)
(define-key quilt-mode-map "\C-c.P" 'quilt-push-all)
(define-key quilt-mode-map "\C-c.O" 'quilt-pop-all)
(define-key quilt-mode-map "\C-c.g" 'quilt-goto)
(define-key quilt-mode-map "\C-c.A" 'quilt-applied)
(define-key quilt-mode-map "\C-c.n" 'quilt-new)
(define-key quilt-mode-map "\C-c.i" 'quilt-import)
(define-key quilt-mode-map "\C-c.a" 'quilt-add)
(define-key quilt-mode-map "\C-c.e" 'quilt-edit-patch)
(define-key quilt-mode-map "\C-c.m" 'quilt-patches)
(define-key quilt-mode-map "\C-c.u" 'quilt-unapplied)
(define-key quilt-mode-map "\C-c.r" 'quilt-refresh)
(define-key quilt-mode-map "\C-c.R" 'quilt-remove)
(define-key quilt-mode-map "\C-c.s" 'quilt-series)
(define-key quilt-mode-map "\C-c.S" 'quilt-edit-series)
(define-key quilt-mode-map "\C-c.h" 'quilt-header)
(define-key quilt-mode-map "\C-c.H" 'quilt-edit-header)
(define-key quilt-mode-map "\C-c.D" 'quilt-delete)

(defvar quilt-mode nil)
(make-variable-buffer-local 'quilt-mode)
(defvar quilt-edit-top-only 't)

(defun quilt-mode (&optional arg)
  "Toggle quilt-mode. With positive arg, enable quilt-mode.

\\{quilt-mode-map}
"
  (interactive "P")
  (setq quilt-mode
	(if (null arg)
	    (not quilt-mode)
	  (> (prefix-numeric-value arg) 0)))
  (if quilt-mode
      (let ((f (quilt-buffer-file-name-safe)))
	(if (quilt-owned-p f)
	    (if (not (quilt-editable f))
		(toggle-read-only 1)
	      (toggle-read-only 0)))
	(quilt-update-modeline))))

(defun quilt-hook ()
  "Enable quilt mode for quilt-controlled files."
  (if (quilt-p) (quilt-mode 1)))

(add-hook 'find-file-hooks 'quilt-hook)
(add-hook 'after-revert-hook 'quilt-hook)

(or (assq 'quilt-mode minor-mode-alist)
    (setq minor-mode-alist
	  (cons '(quilt-mode quilt-mode-line) minor-mode-alist)))

(or (assq 'quilt-mode-map minor-mode-map-alist)
    (setq minor-mode-map-alist
	  (cons (cons 'quilt-mode quilt-mode-map) minor-mode-map-alist)))
