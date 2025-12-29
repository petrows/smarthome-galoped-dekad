# Function for version with logarithmic scale
# Dead zone: 5°
# Normal range: 5° to 185°, values 400 to 1000
# Logarithmic range: 185° to 317°, values 1000 to 5000

# Calculated Logarithmic Scale Values:
# Angle: 185° to 317°, step 1°
# Values: 1000 to 5000
var log_v = [1000,1012,1025,1037,1050,1063,1076,1089,1102,1116,1130,1144,1158,1172,1186,1201,1215,1230,1245,1261,1276,1292,1308,1324,1340,1356,1373,1390,1407,1424,1442,1459,1477,1495,1514,1532,1551,1570,1589,1609,1629,1649,1669,1689,1710,1731,1752,1774,1795,1817,1840,1862,1885,1908,1932,1955,1979,2004,2028,2053,2078,2104,2130,2156,2182,2209,2236,2263,2291,2319,2348,2377,2406,2435,2465,2495,2526,2557,2588,2620,2652,2685,2718,2751,2785,2819,2854,2889,2924,2960,2996,3033,3070,3108,3146,3185,3224,3263,3303,3344,3385,3426,3468,3511,3554,3597,3642,3686,3732,3777,3824,3871,3918,3966,4015,4064,4114,4164,4215,4267,4319,4372,4426,4480,4535,4591,4647,4704,4762,4820,4880,4939,5000]

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
   var a2 = real(185 + i)
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
  pos = 60 + (3.6 * (real(value) - 400.0))
 else
  pos = int(get_logf_angle(value) * 12.0)
 end
 return int(pos)
end
