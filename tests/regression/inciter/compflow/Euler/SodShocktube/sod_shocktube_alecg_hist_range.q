# vim: filetype=sh:
# This is a comment
# Keywords are case-sensitive

title "Sod shock-tube with history output time ranges"

inciter

  term 0.2    # Max physical time
  ttyi 1      # TTY output interval
  cfl 0.1

  scheme alecg

  partitioning
    algorithm mj
  end

  compflow
    depvar u
    physics euler
    problem sod_shocktube

    material
      gamma 1.4 end
    end

    bc_sym
      sideset 2 4 5 6 end
    end
  end

  field_output
    interval 10000
    var
      density "density_numerical"
      x-velocity "x-velocity_numerical"
      y-velocity "y-velocity_numerical"
      z-velocity "z-velocity_numerical"
      specific_total_energy "specific_total_energy_numerical"
      pressure "pressure_numerical"
   end
  end

  diagnostics
    interval  1
    format    scientific
    error l2
  end

  history_output
    interval 20
    time_interval 4.0e-3
    time_range 0.1 0.15 1.0e-3 end
    time_range 0.05 0.12 1.0e-5 end
    point p1 0.1 0.05 0.025 end
    point p2 0.9 0.05 0.025 end
  end

end
