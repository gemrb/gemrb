if [[ ! -d admin ]]; then
  echo 'Run me from the top gemrb dir that contains the admin subdir!'
  exit 1
fi
docdir=gemrb/docs/en/GUIScript

cd admin
methods=$(./check_gui_doc.pl | grep ^+)
cd -

#+ SwapPCs : $md5_hash : SwapPCs(idx1, idx2)\n\n : Swaps the party order
#$1 $2    $3    $4    $5          $6                         $7
while read line; do
  set -- $line
  method=$2
  shift 5
  tmp="$@"

  prototype=$(cut -d: -f1 <<< "$tmp" | sed 's,nn\s*$,,')
  description=$(cut -d: -f2- <<< "$tmp")

  echo Adding $method stub with: $description
  cp $docdir/doc_template.txt $docdir/$method.txt

  return_value=$(awk '{ if(split($0,a,"=>")) print a[2]; }' <<< "$prototype")
  if [[ -n $return_value ]]; then
    # we also have the return "value", so remove it from the prototype
    prototype=$(awk '{ if(split($0,a,"=>")) print a[1]; }' <<< "$prototype")
    sed -i "s@^Return value:@& ${return_value# }@" $docdir/$method.txt
  fi
  sed -i "s@^Prototype:@& GemRB.${prototype% }@" $docdir/$method.txt
  sed -i "s@^Description:@& $description@" $docdir/$method.txt

  # get the parameters
  # eg: WindowIndex ControlID x y w h direction[ font]
  unset parameters
  for parameter in $(sed -n 's@^[^(]*(\([^)]*\)).*$@\1@p' <<< "$prototype"); do
    parameter=$(tr -d ',' <<< "$parameter")
    if [[ ${parameter:${#parameter}-1:1} == "]" ]]; then
      # optional parameter
      parameters="$parameters\n${parameter//]/} - (optional) "
    else
      parameters="$parameters\n${parameter//[/} - "
    fi
  done
  sed -i "s@^Parameters:@& $parameters@" $docdir/$method.txt
done <<< "$methods"
