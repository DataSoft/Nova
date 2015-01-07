package com.datasoft.ceres;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.widget.LinearLayout;

public class DetailsLayout extends LinearLayout
{
	private static final int INVALID_POINTER_ID = -1;
	
	private int m_activePointerId = INVALID_POINTER_ID;
	
	private float m_lastTouchX;
	private float m_lastTouchY;
	private float m_scaleStartX;
	private float m_scaleStartY;
	private float m_scaleFactor = 1f;
	private float m_posX = 0;
	private float m_posY = 0;
 
	private ScaleGestureDetector m_scaleGesture;
	private GestureDetector m_clickGesture;
	
	public DetailsLayout(Context context)
	{
		this(context, null);
		m_scaleGesture = new ScaleGestureDetector(context, new ScaleListener());
		m_clickGesture = new GestureDetector(context, new ClickListener());
		setWillNotDraw(false);
	}
	
	public DetailsLayout(Context context, AttributeSet attrs)
	{
		super(context, attrs);
		m_scaleGesture = new ScaleGestureDetector(context, new ScaleListener());
		m_clickGesture = new GestureDetector(context, new ClickListener());
		setWillNotDraw(false);
	}
	
	@Override
	public void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		
		canvas.scale(m_scaleFactor, m_scaleFactor);
		canvas.translate(m_posX, m_posY);
	}
	
	@Override
	public boolean onTouchEvent(MotionEvent ev)
	{
		m_scaleGesture.onTouchEvent(ev);
		m_clickGesture.onTouchEvent(ev);
		final int action  = ev.getAction();
		switch(action & MotionEvent.ACTION_MASK)
		{
			case MotionEvent.ACTION_DOWN:
			{
				m_lastTouchX = ev.getX();
				m_lastTouchY = ev.getY();
				
				m_activePointerId = ev.getPointerId(0);
				break;
			}
			case MotionEvent.ACTION_MOVE:
			{
				final int pointerIndex = ev.findPointerIndex(m_activePointerId);
				final float dx = ev.getX(pointerIndex) - m_lastTouchX;
				final float dy = ev.getY(pointerIndex) - m_lastTouchY;
				
				if(!m_scaleGesture.isInProgress() && (m_scaleFactor != 1))
				{
					m_posX += dx / m_scaleFactor;
					m_posY += dy / m_scaleFactor;
					
					invalidate();
				}
				
				m_lastTouchX = ev.getX();
				m_lastTouchY = ev.getY();
				
				break;
			}
			case MotionEvent.ACTION_UP:
			{
				m_activePointerId = INVALID_POINTER_ID;
				break;
			}
			case MotionEvent.ACTION_CANCEL:
			{
				m_activePointerId = INVALID_POINTER_ID;
				break;
			}
			case MotionEvent.ACTION_POINTER_UP:
			{
				final int pointerIndex = (action & MotionEvent.ACTION_POINTER_INDEX_MASK)
						>> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
				final int pointerID = ev.getPointerId(pointerIndex);
				if(pointerID == m_activePointerId)
				{
					final int newPointerIndex = pointerIndex == 0 ? 1 : 0;
					m_lastTouchX = ev.getX(newPointerIndex);
					m_lastTouchY = ev.getY(newPointerIndex);
					m_activePointerId = ev.getPointerId(newPointerIndex);
				}
				break;
			}
		}
		
		return true;
	}
	
	private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener
	{
		@Override
		public boolean onScaleBegin(ScaleGestureDetector detector)
		{
			m_scaleFactor *= detector.getScaleFactor();
	        // Don't let the zoom get too small or too large.
			m_scaleFactor = Math.max(0.5f, Math.min(m_scaleFactor, 3.0f));

			float focusX = detector.getFocusX();
			float focusY = detector.getFocusY();
			
			m_scaleStartX = (focusX / m_scaleFactor) - m_posX;
			m_scaleStartY = (focusY / m_scaleFactor) - m_posY;
			return true;
		}
		
		@Override
		public boolean onScale(ScaleGestureDetector detector)
		{
			m_scaleFactor *= detector.getScaleFactor();
	        // Don't let the zoom get too small or too large.
			m_scaleFactor = Math.max(0.5f, Math.min(m_scaleFactor, 3.0f));

			float focusX = detector.getFocusX();
			float focusY = detector.getFocusY();
			
			m_posX = (focusX / m_scaleFactor) - m_scaleStartX;
			m_posY = (focusY / m_scaleFactor) - m_scaleStartY;
			
			invalidate();
			return true;
		}
	}
	
	private class ClickListener extends GestureDetector.SimpleOnGestureListener
	{
		@Override
		public boolean onDoubleTap(MotionEvent ev)
		{
			m_posX = 0;
			m_posY = 0;
			m_scaleFactor = 1;
			invalidate();
			return true;
		}
	}
}
