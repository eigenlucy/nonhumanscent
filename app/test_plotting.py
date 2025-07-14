"""
Test script for demonstrating the plotting utilities.
This script loads the sensor data from CSV files and creates various plots.
"""

import os
import sys
import subprocess
import plotly
import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import plotly.io as pio

# Import plotting utilities
from plot_utils import (
    load_csv_data,
    preprocess_data,
    plot_time_series,
    plot_multi_series,
    create_plotly_figure,
    plot_comparison,
    detect_events,
    create_dashboard
)

# Set Plotly to open plots in the browser
pio.renderers.default = "browser"

def align_time_series(dfs):
    """
    Align multiple dataframes to use the same time range based on the shortest dataset.
    
    Args:
        dfs: Dictionary of DataFrames with sensor data
        
    Returns:
        Dictionary of aligned DataFrames
    """
    if not dfs:
        return {}
    
    # Find the shortest time range
    min_start = None
    max_end = None
    
    for df in dfs.values():
        if 'timestamp' not in df.columns:
            continue
            
        start = df['timestamp'].min()
        end = df['timestamp'].max()
        
        if min_start is None or start > min_start:
            min_start = start
        
        if max_end is None or end < max_end:
            max_end = end
    
    # Align all dataframes to this range
    aligned_dfs = {}
    for name, df in dfs.items():
        if 'timestamp' not in df.columns:
            aligned_dfs[name] = df
            continue
            
        aligned_df = df[(df['timestamp'] >= min_start) & (df['timestamp'] <= max_end)]
        aligned_dfs[name] = aligned_df
    
    return aligned_dfs

def main():
    """Main function to demonstrate plotting utilities."""
    print("Loading sensor data...")
    
    # Define paths to CSV files
    csv_dir = os.path.join("data", "csv_output")
    combined_csv = os.path.join(csv_dir, "combined_scents.csv")
    individual_csvs = {
        "lavender": os.path.join(csv_dir, "lavender.csv"),
        "lemongrass": os.path.join(csv_dir, "lemongrass.csv"),
        "orange": os.path.join(csv_dir, "orange.csv"),
        "street-air": os.path.join(csv_dir, "street-air.csv")
    }
    
    # Create output directory if it doesn't exist
    os.makedirs(os.path.join(csv_dir, "plots"), exist_ok=True)
    
    # Load the combined dataset
    if os.path.exists(combined_csv):
        print(f"Loading combined data from {combined_csv}")
        combined_df = load_csv_data(combined_csv)
        print(f"Loaded {len(combined_df)} records")
        
        # Basic preprocessing
        print("Preprocessing data...")
        smoothed_df = preprocess_data(combined_df, smooth_window=5)
        
        # Create and show various plots
        print("Creating plots...")
        
        # 1. Basic time series plot of gas resistance by scent
        print("1. Creating time series plot of gas resistance...")
        fig1 = plot_time_series(
            smoothed_df, 
            y_column="gas_resistance", 
            title="Gas Resistance Over Time by Scent",
            color_by="scent",
            log_scale=True
        )
        fig1.write_html(os.path.join(csv_dir, "plots", "gas_resistance_time_series.html"))
        fig1.show()
        
        # 2. Multi-panel plot with all sensor readings
        # print("2. Creating multi-panel plot with all sensor readings...")
        # fig2 = plot_multi_series(
        #     smoothed_df,
        #     columns=["gas_resistance", "temperature", "humidity"],
        #     title="Sensor Readings Over Time",
        #     color_by="scent"
        # )
        # fig2.write_html(os.path.join(csv_dir, "plots", "multi_panel_plot.html"))
        # fig2.show()
        
        # 3. Scatter plot of gas resistance vs. humidity colored by scent
        print("3. Creating scatter plot of gas resistance vs. humidity...")
        fig3 = create_plotly_figure(
            smoothed_df,
            plot_type="scatter",
            x="humidity",
            y="gas_resistance",
            color="scent",
            title="Gas Resistance vs. Humidity",
            log_y=True,
            opacity=0.7,
            size_max=10
        )
        fig3.write_html(os.path.join(csv_dir, "plots", "gas_vs_humidity_scatter.html"))
        fig3.show()
        
        # 4. Box plot of gas resistance by scent
        print("4. Creating box plot of gas resistance by scent...")
        fig4 = create_plotly_figure(
            smoothed_df,
            plot_type="box",
            x="scent",
            y="gas_resistance",
            title="Distribution of Gas Resistance by Scent",
            log_y=True
        )
        fig4.write_html(os.path.join(csv_dir, "plots", "gas_resistance_boxplot.html"))
        fig4.show()
        
        # 5. Event detection - high gas resistance events
        print("5. Detecting high gas resistance events...")
        # Calculate a threshold based on percentile
        threshold = smoothed_df["gas_resistance"].quantile(0.75)
        events_df = detect_events(
            smoothed_df,
            column="gas_resistance",
            threshold=threshold,
            window=3,
            direction="above"
        )
        
        # Plot events
        fig5 = create_plotly_figure(
            events_df,
            plot_type="scatter",
            x="timestamp",
            y="gas_resistance",
            color="event",
            title=f"Gas Resistance Events (Threshold: {threshold:.2f})",
            log_y=True
        )
        fig5.write_html(os.path.join(csv_dir, "plots", "gas_resistance_events.html"))
        fig5.show()
        
        # 6. Create a comprehensive dashboard
        print("6. Creating comprehensive dashboard...")
        fig6 = create_dashboard(smoothed_df)
        fig6.write_html(os.path.join(csv_dir, "plots", "sensor_dashboard.html"))
        fig6.show()
        
    else:
        print(f"Combined CSV file not found: {combined_csv}")
        
        # Load individual datasets if available
        dfs = {}
        for name, path in individual_csvs.items():
            if os.path.exists(path):
                print(f"Loading {name} data from {path}")
                dfs[name] = load_csv_data(path)
        
        if dfs:
            # Align time series to use the shortest data length
            print("Aligning time series data...")
            aligned_dfs = align_time_series(dfs)
            
            # Preprocess each dataset
            processed_dfs = {name: preprocess_data(df, smooth_window=5) 
                           for name, df in aligned_dfs.items()}
            
            # Create comparison plot
            print("Creating comparison plot of gas resistance...")
            fig = plot_comparison(
                processed_dfs,
                y_column="gas_resistance",
                title="Comparison of Gas Resistance Across Scents",
                log_scale=True
            )
            fig.write_html(os.path.join(csv_dir, "plots", "scent_comparison.html"))
            fig.show()
        else:
            print("No data files found.")

if __name__ == "__main__":
    main()
