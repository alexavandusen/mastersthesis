#!/usr/bin/env python3
"""
compare_tracks2.py download Google walking directions and report deviation.
"""
import argparse
import sys
import signal

import googlemaps
import polyline
import numpy as np
import pandas as pd

sys.setrecursionlimit(100000)


import folium
import gpxpy

from shapely.geometry import LineString, Point
from pyproj import Transformer
from frechetdist import frdist

# Prevent BrokenPipeError when piping output
signal.signal(signal.SIGPIPE, signal.SIG_DFL)


def get_args():
    parser = argparse.ArgumentParser(
        description="Compare GPS track to Google walking route"
    )
    parser.add_argument(
        "--key", required=True,
        help="Google Maps API key"
    )
    parser.add_argument(
        "--track", required=True,
        help="Path to CSV or GPX file with lat,lon data"
    )
    parser.add_argument(
        "--origin", required=True,
        help="Origin coordinate as 'lat,lon'"
    )
    parser.add_argument(
        "--destination", required=True,
        help="Destination coordinate as 'lat,lon'"
    )
    parser.add_argument(
        "--waypoints", default="",
        help="Waypoints as 'lat,lon|lat,lon' (optional)"
    )
    return parser.parse_args()


def fetch_google_polyline(key, origin, destination, waypoints):
    client = googlemaps.Client(key)
    wpts = waypoints.split("|") if waypoints else None
    response = client.directions(
        origin,
        destination,
        mode="walking",
        waypoints=wpts,
        alternatives=False
    )
    if not response:
        raise RuntimeError(
            "No route returned – check API key, quota, or coordinates"
        )
    return polyline.decode(
        response[0]["overview_polyline"]["points"]
    )


def load_gpx(path):
    with open(path, "r") as f:
        gpx = gpxpy.parse(f)
    points = []
    for track in gpx.tracks:
        for segment in track.segments:
            for p in segment.points:
                points.append((p.latitude, p.longitude))
    return points


def metric_proj(coords_ll):
    # Project WGS84 lat/lon to UTM zone 30N (metres)
    transformer = Transformer.from_crs(
        "epsg:4326", "epsg:32630", always_xy=True
    )
    return [transformer.transform(lon, lat) for lat, lon in coords_ll]


def resample_curve(coords_xy, num_points):
    line = LineString(coords_xy)
    distances = np.linspace(0, line.length, num_points)
    return [
        (pt.x, pt.y)
        for pt in (line.interpolate(d) for d in distances)
    ]


def main():
    args = get_args()

    # 1. Fetch Google route
    google_ll = fetch_google_polyline(
        args.key, args.origin, args.destination, args.waypoints
    )
    google_xy = metric_proj(google_ll)
    google_line = LineString(google_xy)

    # 2. Load recorded track
    if args.track.lower().endswith(".gpx"):
        track_ll = load_gpx(args.track)
    else:
        df = pd.read_csv(args.track)
        df = df.rename(
            columns={"latitude": "lat", "longitude": "lon"}
        )
        track_ll = df[["lat", "lon"]].values.tolist()

    track_xy = metric_proj(track_ll)
    track_line = LineString(track_xy)

    # 3. Distance metrics
    distances = [
        google_line.distance(Point(x, y))
        for x, y in track_xy
    ]
    mean_distance = np.mean(distances)
    p95_distance = np.percentile(distances, 95)

    num_points = max(len(google_xy), len(track_xy))
    google_rs = resample_curve(google_xy, num_points)
    track_rs = resample_curve(track_xy, num_points)
    frechet_distance = frdist(google_rs, track_rs)

    length_ratio = track_line.length / google_line.length

    # 4. Deviation report
    print("Deviation Report")
    print(f"Mean perpendicular distance : {mean_distance:.1f} m")
    print(f"95th-percentile distance    : {p95_distance:.1f} m")
    print(f"Fréchet (worst-case)        : {frechet_distance:.1f} m")
    print(f"Path-length ratio           : {length_ratio:.2f}×")
    print()
    pd.DataFrame([{
    'mean_distance':       mean_distance,
    'p95_distance':        p95_distance,
    'frechet_distance':    frechet_distance,
    'path_length_ratio':   length_ratio
    }]).to_csv('deviation_summary.csv', index=False)

    # Debug lengths
    print(
        f"Loaded {len(google_ll)} Google points, {len(track_ll)} track points", 
        flush=True
    )

    # 5. Map visualization
    m = folium.Map(location=track_ll[0], zoom_start=15)
    folium.PolyLine(google_ll, color="blue", weight=5, opacity=0.7).add_to(m)
    folium.PolyLine(track_ll, color="red",  weight=5, opacity=0.7).add_to(m)
    m.save("track_compare.html")
    print("Saved map to track_compare.html")


if __name__ == "__main__":
    try:
        main()
    except BrokenPipeError:
        sys.exit(0)
