#!/bin/bash

# Configuration
IMG_DIR="*Your image dir*"
HTML_FILE="$IMG_DIR/*Your html file*"

# Check if HTML file exists
if [ ! -f "$HTML_FILE" ]; then
    echo "Error: $HTML_FILE does not exist"
    exit 1
fi

# Get all jpg images in the directory
cd "$IMG_DIR" || exit 1
jpg_files=(*.jpg)

# Check if any jpg files exist
if [ ${#jpg_files[@]} -eq 0 ] || [ ! -e "${jpg_files[0]}" ]; then
    echo "No .jpg files found in $IMG_DIR"
    exit 0
fi

# Extract existing image filenames from HTML
existing_images=$(grep -o 'src="[^"]*\.jpg"' "$HTML_FILE" 2>/dev/null | sed 's/src="//;s/"//' || echo "")

# Find the highest location number currently in use
highest_location=$(grep -o 'id="location[0-9]*"' "$HTML_FILE" | sed 's/id="location//;s/"//' | sort -n | tail -1)
if [ -z "$highest_location" ]; then
    next_location=1
else
    next_location=$((highest_location + 1))
fi

# Counter for new images added
new_images_count=0

# Process each jpg file
for img_file in "${jpg_files[@]}"; do
    # Check if this image is already in the HTML
    if echo "$existing_images" | grep -q "^$img_file$"; then
        continue
    fi
    
    # Get filename without extension for the title
    filename_no_ext="${img_file%.jpg}"
    
    # Create the new content block
    new_block="    <div id=\"location$next_location\" class=\"content-display\">
        <h2 class=\"location-title\">$filename_no_ext</h2>
        <div class=\"description\">
            $filename_no_ext
        </div>
        <div class=\"image-container\">
            <img src=\"$img_file\" alt=\"$filename_no_ext\">
        </div>
    </div>"
    
    # Use awk to insert after the last location div
    awk -v new_block="$new_block" '
    # Store all lines
    { lines[NR] = $0 }
    
    # Track when we see a location div closing
    /id="location[0-9]+"/ { last_location_start = NR }
    
    END {
        # Calculate where the location block ends (8 lines after it starts based on your structure)
        if (last_location_start > 0) {
            insert_line = last_location_start + 8
        } else {
            insert_line = NR - 5  # Fallback
        }
        
        # Print everything up to insert point
        for (i = 1; i <= insert_line; i++) {
            print lines[i]
        }
        
        # Print blank line and new block
        print ""
        print new_block
        
        # Print rest of file
        for (i = insert_line + 1; i <= NR; i++) {
            print lines[i]
        }
    }
    ' "$HTML_FILE" > "$HTML_FILE.tmp"
    
    mv "$HTML_FILE.tmp" "$HTML_FILE"
    
    # Now add the dropdown option for this new location
    # Find the line with </select> and insert the new option before it
    dropdown_option="                    <option value=\"location$next_location\">$filename_no_ext</option>"
    
    awk -v new_option="$dropdown_option" '
    {
        # If we find the closing </select> tag, insert our option before it
        if ($0 ~ /<\/select>/) {
            print new_option
        }
        print $0
    }
    ' "$HTML_FILE" > "$HTML_FILE.tmp"
    
    mv "$HTML_FILE.tmp" "$HTML_FILE"
    
    echo "Added $img_file as location$next_location with dropdown option"
    next_location=$((next_location + 1))
    new_images_count=$((new_images_count + 1))
done

if [ $new_images_count -eq 0 ]; then
    echo "No new images to add. All images already in HTML."
else
    echo "Successfully added $new_images_count new image(s) to $HTML_FILE"
fi