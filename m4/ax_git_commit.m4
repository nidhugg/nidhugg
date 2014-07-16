#
# AX_GIT_COMMIT
#
# Attempts to find a short hash for the HEAD commit of the build
# directory. On success GIT_COMMIT is defined to that hash, and the
# shell variable GIT_COMMIT is set to that hash.
#

AC_DEFUN([AX_GIT_COMMIT],
[
  GIT_COMMIT=''
  ax_git_commit_ok='yes'
  AC_PATH_PROG([GIT],[git],[no])

  if test "x$GIT" = "xno"; then
    ax_git_commit_ok='no'
    AC_MSG_NOTICE([Unable to find git commit of build repository.])
  fi

  if test "x$ax_git_commit_ok" = "xyes"; then
    AC_MSG_CHECKING([git commit of build repository])

    GIT_COMMIT=`$GIT log -n 1 '--pretty=format:%h' 2>/dev/null`
    if test "$?" -ne 0; then
      ax_git_commit_ok='no'
    fi

    if test "x$ax_git_commit_ok" = "xyes"; then
      AC_MSG_RESULT([$GIT_COMMIT]);
      AC_DEFINE_UNQUOTED([GIT_COMMIT],["$GIT_COMMIT"],[The hash of the HEAD commit in the build repository.])
    else
      AC_MSG_RESULT([no])
    fi
  fi
])
