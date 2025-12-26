# Function for version with logarithmic scale
# Dead zone: 5°
# Normal range: 5° to 190°, values 400 to 1000
# Logarithmic range: 190° to 315°, values 1000 to 5000

# Calculated Logarithmic Scale Values:
# Angle: 190° to 315°, step 1°
# Values: 1000 to 5000
var log_v = [1000,1013,1026,1039,1053,1066,1080,1094,1108,1123,1137,1152,1167,1182,1198,1213,1229,1245,1261,1277,1294,1310,1327,1345,1362,1380,1398,1416,1434,1453,1471,1491,1510,1529,1549,1569,1590,1610,1631,1652,1674,1695,1717,1740,1762,1785,1808,1832,1855,1879,1904,1928,1953,1979,2004,2030,2057,2083,2110,2138,2165,2193,2222,2251,2280,2309,2339,2369,2400,2431,2463,2495,2527,2560,2593,2627,2661,2695,2730,2765,2801,2837,2874,2911,2949,2987,3026,3065,3105,3145,3186,3227,3269,3312,3354,3398,3442,3487,3532,3578,3624,3671,3718,3767,3815,3865,3915,3966,4017,4069,4122,4175,4229,4284,4340,4396,4453,4511,4569,4628,4688,4749,4811,4873,4936,5000]

def get_logf_angle(value)
 if value < log_v[0]
  return log_v[0]
 end
 if value > log_v[-1]
  return log_v[-1]
 end

 # Find next value greater than we
 for i : 0 .. size(log_v)
  if value <= log_v[i]
   # Linear interpolation between log_v[i-1] and log_v[i]
   var v1 = real(log_v[i-1])
   var v2 = real(log_v[i])
   # Current agnle-1 and angle
   var a2 = real(190 + i)
   var a1 = a2 - 1.0
   var angle = a1 + (value - v1) * (a2 - a1) / (v2 - v1)
   return angle
  end
 end
 return nil
end

# Function to get drive position from value
def get_drive_pos(value)
 var pos = 0
 if value < 400
  value = 400
 end
 if value > 5000
  value = 5000
 end
 # Linear scale?
 if value <= 1000
  # Dead zone (5*12 steps) + linear (185*12 steps / 600 units range)
  pos = 60 + (3.7 * (real(value) - 400.0))
 else
  pos = int(get_logf_angle(value) * 12.0)
 end
 return int(pos)
end
