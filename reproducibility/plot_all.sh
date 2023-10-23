date
start_time=$(date +%s)


# plot Figure 1
cd fig1-scripts
python plot_fig1.py
cd ..

# plot Figure 3
cd fig3-scripts
python plot_fig3.py
cd ..

# plot Figure 4a
cd fig4a-scripts
python plot_fig4a.py
cd ..

# plot Figure 4b
cd fig4b-scripts
python plot_fig4b.py
cd ..

# plot Figure 5
cd fig5-scripts
python plot_fig5.py
cd ..

# plot Figure 6
cd fig6-scripts
plot plot_fig6.py
cd ..

# plot Figure 7
cd fig7-scripts
plot plot_fig7.py
cd ..

# plot Figure 8(a)
cd fig8a-scripts
plot plot_fig8a.py
cd ..

# plot Figure 8(b)
cd fig8b-scripts
plot plot_fig8b.py
cd ..

# plot Figure 8(c)
cd fig8c-scripts
plot plot_fig8c.py
cd ..

# plot Figure 8(d)
cd fig8d-scripts
plot plot_fig8d.py
cd ..

# plot Figure 9
cd fig9-scripts
plot plot_fig9.py
cd ..

# plot Figure 10
cd fig10-scripts
plot plot_fig10a.py
plot plot_fig10b.py
cd ..


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date