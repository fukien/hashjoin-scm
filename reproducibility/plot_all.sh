# Record the start time for plotting
start_time=$(date +%s)

# Plot Figure 1
cd fig1-scripts
python plot_fig1.py
cd ..

# Plot Figure 3
cd fig3-scripts
python plot_fig3.py
cd ..

# Plot Figure 4a
cd fig4a-scripts
python plot_fig4a.py
cd ..

# Plot Figure 4b
# cd fig4b-scripts
# python plot_fig4b.py
# cd ..

# Plot Figure 5
cd fig5-scripts
python plot_fig5.py
cd ..

# Plot Figure 6
cd fig6-scripts
python plot_fig6.py
cd ..

# Plot Figure 7
cd fig7-scripts
python plot_fig7.py
cd ..

# Plot Figure 8(a)
cd fig8a-scripts
python plot_fig8a.py
cd ..

# Plot Figure 8(b)
cd fig8b-scripts
python plot_fig8b.py
cd ..

# Plot Figure 8(c)
cd fig8c-scripts
python plot_fig8c.py
cd ..

# Plot Figure 8(d)
cd fig8d-scripts
python plot_fig8d.py
cd ..

# Plot Figure 9
cd fig9-scripts
python plot_fig9.py
cd ..

# Plot Figure 10
cd fig10-scripts
python plot_fig10a.py
python plot_fig10b.py
cd ..

# Record the end time for plotting
end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"

# Display the current date and time
date
