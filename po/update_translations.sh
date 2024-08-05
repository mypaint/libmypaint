#!/usr/bin/env bash

update_translations()
{
    cd $(dirname "$1")
    local HELP
    HELP="\
====
Update translation files: generated sources / po template / po language files
Usage: $1 [--force] [[--only-template] | [LANG...]]

If no languages are specified, and --only-template is not set, all .po files in
the directory will be updated, same as running: $1 *.po
====
"
    shift

    sec() { date -r "$1" "+%s"; }
    err() { >&2 echo -e "\e[91m""Error: $@""\e[0m"; }
    print_errors()
    {
        for e in "$@"
        do
            err "$e"
        done
    }

    local ORIG GEN_SRC ENUM GEN TEMPLATE FORCE ONLY_TEMPLATE
    ORIG=../brushsettings.json
    GEN_SRC=../brushsettings-gen.h
    ENUM=../mypaint-brush-settings-gen.h
    GEN_SCRIPT=../generate.py
    TEMPLATE=libmypaint.pot

    local langs errors
    langs=()
    errors=()

    while [ -n "$1" ]
    do
        case "$1" in
            --help)
                echo "$HELP" && exit 0
                ;;
            --force)
                FORCE=1
                ;;
            --only-template)
                ONLY_TEMPLATE=1
                ;;
            -*)
                errors+=("Unrecognized option: $1")
                ;;
            *)
                local f
                f="${1%%.po}.po"
                if [ ! -e "$f" ]
                then
                    errors+=("Not found: $f - LANG must be the code or .po file for an existing language")
                else
                    langs+=("$f")
                fi
                ;;
        esac
        shift
    done

    # Sanity check
    if [ -n "$ONLY_TEMPLATE" -a -n "$langs" ]
    then
        errors+=("Don't specify languages when using ``--only-template``")
    fi
    # Print usage instructions followed by error message(s)
    [ -n "$errors" ] && >&2 echo "$HELP" && print_errors "${errors[@]}" && exit 1

    # Check if the message source file needs to be (re)generated.
    # ( generated source: not present,older than basis, older than script )
    if [ -n "$FORCE" -o ! -e "$GEN_SRC" -o \
            $(sec "$GEN_SRC") -lt $(sec "$ORIG") -o  \
            $(sec "$GEN_SRC") -lt $(sec "$GEN_SCRIPT") ]
    then
        [ -z "$FORCE" ] &&
            echo "Generated file missing or out of date, generating..." ||
                echo "Generating (forced)..."
        python "$GEN_SCRIPT" "$ENUM" "$GEN_SRC" ||
            (echo "Failed to generate source file!" && exit 1)
    fi

    # Check if the template file appears up to date
    if [ -z "$FORCE" -a -e "$TEMPLATE" -a $(sec "$GEN_SRC") -lt $(sec "$TEMPLATE") ]
    then
        echo "$TEMPLATE up to date, skipping extraction (use --force to override)."
    else
        local temp_template temp_diff
        temp_template=$(mktemp)
        temp_diff=$(mktemp)
        # Omit locations from the generated file, and instead...
        xgettext --no-location -c -kN_:1 -o - "$GEN_SRC" |
            # ...transform special generated comments into accurate source locations.
            sed -E "s@^#\. (: $ORIG:.*)@#\1@" > "$temp_template"
        # Don't update template if the only change is the creation date
        diff --suppress-common-lines -y "$TEMPLATE" "$temp_template" > "$temp_diff"
        if [ $(wc -l < "$temp_diff") -eq 1 -a
             $(grep -i -o "POT-Creation-Date" "$temp_diff" | wc -l) -eq 2 ]
        then
            echo "$TEMPLATE unchanged"
        else
            mv "$temp_template" "$TEMPLATE" && echo "$TEMPLATE updated."
        fi
    fi

    # If requested, don't update any languages
    [ -n "$ONLY_TEMPLATE" ] && exit 0

    # If no languages are specified, try to update all of them
    if [ -z "${langs[*]}" ]
    then
        langs=($(ls *.po))
    fi

    local failed_updates
    failed_updates=()

    # Update the language files based on the template
    for lang in "${langs[@]}"
    do
        msgmerge -q -U "$lang" "$TEMPLATE" || failed_updates+=("$lang")
    done

    echo "Successfully processed $((${#langs[@]} - ${#failed_updates[@]})) language files."
    if [ -n "${failed_updates[*]}" ]
    then
        err "Failed to update ${#failed_updates[@]} language files:"
        for f in "${failed_updates[@]}"
        do
            err "$f"
        done
        exit 1
    fi
}

update_translations "$0" "$@"
