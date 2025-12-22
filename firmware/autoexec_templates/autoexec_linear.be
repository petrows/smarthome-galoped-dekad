# Function for version with linear scale
# Dead zone: 15°
# Normal range: 15° to 315°, values 400 to 2200

# Function to get drive position from value
def get_drive_pos(value)
 if value < 400
  value = 400
 end
 if value > 2200
  value = 2200
 end
 # Dead zone (15*12 steps) + linear (300*12 steps / 1800 units range)
 return 180 + ((int(value) - 400) * 2)
end
